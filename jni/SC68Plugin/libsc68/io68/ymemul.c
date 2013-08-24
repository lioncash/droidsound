/*
 * @file    ym_emul.c
 * @brief   YM-2149 emulator
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (C) 1998-2013 Benjamin Gerard
 *
 * Time-stamp: <2013-08-16 07:24:48 ben>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_CONFIG_OPTION68_H
# include "config_option68.h"
#else
# include "default_option68.h"
#endif

#include "ymemul.h"
#include "emu68/assert68.h"

#include <sc68/file68_msg.h>
#include <sc68/file68_opt.h>

#include <string.h>

#ifndef BREAKPOINT68
# define BREAKPOINT68 assert(!"breakpoint")
#endif

#ifndef DEBUG_YM_O
# define DEBUG_YM_O 0
#endif
int ym_cat = msg68_DEFAULT;

int ym_default_chans = 7;

#include "ym_linear_table.c"
#include "ym_atarist_table.c"

/** 3 channels output table.
 *  Using a table for non linear mixing.
 */
static s16 ymout5[32*32*32];
static inline
void access_list_reset(ym_waccess_list_t * const access_list,
                       const char * name,
                       const cycle68_t ymcycle)
{
  if (!name) name = "und";
  access_list->name[0] = name[0];
  access_list->name[1] = name[1];
  access_list->name[2] = name[2];
  access_list->name[3] = 0;
  access_list->head = access_list->tail = 0;
}

static void access_list_add(ym_t * const ym,
                            ym_waccess_list_t * const access_list,
                            const int reg, const int val,
                            const cycle68_t ymcycle)
{
  ym_waccess_t * free_access = ym->waccess_nxt;

  if (free_access >= ym->waccess+ym->waccess_max) {
    /* No more free entries. */
    /* $$$ TODO: realloc buffer, reloc all lists ... */
    ++ym->overflow;
    return;
  }
  ym->waccess_nxt = free_access+1;

  free_access->ymcycle = ymcycle;
  free_access->reg     = reg;
  free_access->val     = val;
  free_access->link    = 0;

  if (access_list->tail) {
    access_list->tail->link = free_access;
  } else {
    access_list->head = free_access;
  }
  access_list->tail = free_access;
}

static void access_adjust_cycle(ym_waccess_list_t * const access_list,
                                const cycle68_t ymcycles)
{
  ym_waccess_t * access;
  /* access_list->last_cycle -= ymcycles; */
  for (access = access_list->head; access; access = access->link) {
    access->ymcycle -= ymcycles;
  }
}


/******************************************************
 *                  Yamaha reset                       *
 ******************************************************/

int ym_reset(ym_t * const ym, const cycle68_t ymcycle)
{
  int ret = -1;

  static const struct ym2149_reg_s init_regs = {
    0xff, 0x0f, 0xff, 0x0f, 0xff, 0x0f, /* tone period A,B,C */
    0x3f, 077,                          /* noise period & mixer */
    0x00, 0x00, 0x00,                   /* Volume A,B,C */
    0xFF, 0xFF,                         /* envelop period */
    0x0A,                               /* envelop shape */
    0,0                                 /* oi a,b */
  };


  if (ym) {
    /* reset registers */
    ym->shadow.name = ym->reg.name = init_regs;
    ym->ctrl = 0;

    /* Run emulator specific reset callback. */
    if (ym->cb_reset) {
      ym->cb_reset(ym,ymcycle);
    }

    /* Reset access lists */
    access_list_reset(&ym->ton_regs, "Ton", ymcycle);
    access_list_reset(&ym->noi_regs, "Noi", ymcycle);
    access_list_reset(&ym->env_regs, "Env", ymcycle);
    ym->overflow = 0;

    ret = 0;
  }

  return ret;
}


/******************************************************
 *                  Yamaha init                        *
 ******************************************************/

/* Select default engine */
#ifndef YM_ENGINE
# define YM_ENGINE YM_ENGINE_BLEP
#endif

#ifndef DEF_ENGINE_STR
# if YM_ENGINE == YM_ENGINE_BLEP
#  define DEF_ENGINE_STR "[blep*|pulse|dump]"
# elif YM_ENGINE == YM_ENGINE_PULSE
#  define DEF_ENGINE_STR "[pulse*|blep|dump]"
# elif YM_ENGINE == YM_ENGINE_DUMP
#  define DEF_ENGINE_STR "[dump*|pulse|blep]"
# else
#  error unkwown default YM engine
# endif
#endif

/* Select default volume table */
#ifndef YM_VOLMODEL
# define YM_VOLMODEL YM_VOL_ATARIST
#endif

#ifndef DEF_VOLMODEL_STR
# if YM_VOLMODEL == YM_VOL_ATARIST
#  define DEF_VOLMODEL_STR "[atari*|linear]"
# elif YM_VOLMODEL == YM_VOL_LINEAR
#  define DEF_VOLMODEL_STR "[linear*|atari]"
# else
#  error unkwown default YM volume model
# endif
#endif

/* Default parameters */
static ym_parms_t default_parms;

/* Max output level for volume tables. */
static const int output_level = 0xCAFE;

static int onchange_engine(const option68_t *opt, value68_t * val)
{
  int k;

  TRACE68(ym_cat,"ym-2149: change YM engine model to -- *%s*\n", val->str);

  if (!strcmp(val->str,"pulse"))
    k = YM_ENGINE_PULS;
  else if (!strcmp(val->str,"blep"))
    k = YM_ENGINE_BLEP;
  else if (!strcmp(val->str,"dump"))
    k = YM_ENGINE_DUMP;
  else if (!strcmp(val->str,"default"))
    k = YM_ENGINE_DEFAULT;
  else
    return -1;

  ym_engine(0, k);
  return 0;
}

static int onchange_volume(const option68_t *opt, value68_t * val)
{
  if (!strcmp(val->str,"atari") || !strcmp(val->str,"default"))
    default_parms.volmodel = YM_VOL_ATARIST;
  else if (!strcmp(val->str,"linear"))
    default_parms.volmodel = YM_VOL_LINEAR;
  else
    return -1;
  return 0;
}

/* Command line options */
static const char prefix[] = "sc68-";
static const char engcat[] = "ym-2149";
static option68_t opts[] = {
  {
    onchange_engine,
    option68_STR, prefix, "ym-engine", engcat,
    "set ym-2149 engine " DEF_ENGINE_STR },
  {
    onchange_volume,
    option68_STR, prefix, "ym-volmodel", engcat,
    "set ym-2149 volume model " DEF_VOLMODEL_STR },
  {
    0,
    option68_INT, prefix, "ym-chans", engcat,
    "set ym-2149 active channel [bit-0:A ... bit-2:C]" }
};

static const char * ym_engine_name(int emul);
static const char * ym_volmodel_name(int model);


int ym_init(int * argc, char ** argv)
{
  /* Debug */
  ym_cat = msg68_cat("ym","ym-2149 emulator",DEBUG_YM_O);

  /* Setup default */
  default_parms.engine   = YM_ENGINE;
  default_parms.volmodel = YM_VOLMODEL;
  default_parms.clock    = YM_CLOCK_ATARIST;
  default_parms.hz       = SAMPLING_RATE_DEF;

  /* Register ym options */
  option68_append(opts,sizeof(opts)/sizeof(*opts));

  /* Default option values */
  option68_set ( opts+0, ym_engine_name(default_parms.engine) );
  option68_set ( opts+1, ym_volmodel_name(default_parms.volmodel) );
  option68_iset( opts+2, ym_default_chans );

  /* Parse options */
  *argc = option68_parse(*argc,argv,0);

  /* Set volume table (unique for all instance) */
  switch (default_parms.volmodel) {
  case YM_VOL_LINEAR:
    ym_create_5bit_linear_table(ymout5, output_level);
    break;
  /* case YM_VOL_ATARIST_4BIT: */
  /*   ym_create_4bit_atarist_table(ymout5, output_level); */
  /*   break; */
  case YM_VOL_DEFAULT:
  case YM_VOL_ATARIST:
  default:
    ym_create_5bit_atarist_table(ymout5, output_level);
  }

  /* Process ym-puls options */
  *argc = ym_puls_options(*argc, argv);

  return 0;
}

void ym_shutdown(void)
{
}


/* ,-----------------------------------------------------------------.
 * |                         Run emulation                           |
 * `-----------------------------------------------------------------'
 */
int ym_run(ym_t * const ym, s32 * output, const cycle68_t ymcycles)
{
  if (!ymcycles) {
    return 0;
  }

  /*
   * Currently ymcycles must be multiple of 32.
   * 32 cycles will generate 4 samples @ 250000hz. It helps to
   * simplify the first filter code (half-level buzz trick).
   */
  if ( (ymcycles&31) || !output)  {
    return -1;
  }

  return ym->cb_run(ym,output,ymcycles);
}


/* ,-----------------------------------------------------------------.
 * |                         Write YM register                       |
 * `-----------------------------------------------------------------'
 */

void ym_writereg(ym_t * const ym,
                 const int val, const cycle68_t ymcycle)
{
  const int reg = ym->ctrl;

  if (reg >= 0 && reg < 16) {

    /*TRACE68(ym_cat,"write #%X = %02X (%u)\n",reg,(int)(u8)val,ymcycle); */

    ym->shadow.index[reg] = val;

    switch(reg) {
      /* Tone generator related registers. */
    case YM_PERL(0): case YM_PERH(0):
    case YM_PERL(1): case YM_PERH(1):
    case YM_PERL(2): case YM_PERH(2):
    case YM_VOL(0): case YM_VOL(1): case YM_VOL(2):
      access_list_add(ym, &ym->ton_regs, reg, val, ymcycle);
      break;

      /* Envelop generator related registers. */
    case YM_ENVL: case YM_ENVH: case YM_ENVTYPE:
      access_list_add(ym, &ym->env_regs, reg, val, ymcycle);
      break;

      /* Reg 7 modifies both noise &ym-> tone generators */
    case YM_MIXER:
      access_list_add(ym, &ym->ton_regs, reg, val, ymcycle);
      /* Noise generator related registers. */
    case YM_NOISE:
      access_list_add(ym, &ym->noi_regs, reg, val, ymcycle);
      break;

    default:
      break;
    }
  }
}


/* ,-----------------------------------------------------------------.
 * |                  Adjust YM-2149 cycle counters                  |
 * `-----------------------------------------------------------------'
 */
void ym_adjust_cycle(ym_t * const ym, const cycle68_t ymcycles)
{
  if (ym) {
    access_adjust_cycle(&ym->ton_regs, ymcycles);
    access_adjust_cycle(&ym->noi_regs, ymcycles);
    access_adjust_cycle(&ym->env_regs, ymcycles);
  }
}


/* ,-----------------------------------------------------------------.
 * |                  Yamaha get activated voices                    |
 * `-----------------------------------------------------------------'
 */

extern const int ym_smsk_table[];       /* declared in ym_puls.c */
int ym_active_channels(ym_t * const ym, const int clr, const int set)
{
  int v = 0;
  if (ym) {
    const int voice_mute = ym->voice_mute;
    v = ( voice_mute & 1 ) | ( (voice_mute>>5) & 2 ) | ( (voice_mute>>10) & 4);
    v = ( (v & ~clr ) | set ) & 7;
    ym->voice_mute = ym_smsk_table[v];
    msg68_notice("ym-2149: active channels -- *%c%c%c*\n",
               (v&1)?'A':'.', (v&2)?'B':'.', (v&4)?'C':'.');
  }
  return v;
}


/* ,-----------------------------------------------------------------.
 * |                        Engine selection                         |
 * `-----------------------------------------------------------------'
 */

static
const char * ym_engine_name(int emul)
{
  switch (emul) {
  case YM_ENGINE_PULS:    return "pulse";
  case YM_ENGINE_BLEP:    return "blep";
  case YM_ENGINE_DUMP:    return "dump";
  }
  return 0;
}

int ym_engine(ym_t * const ym, int engine)
{
  switch (engine) {

  case YM_ENGINE_QUERY:
    /* Get current value. */
    engine = ym ? ym->engine : default_parms.engine;
    break;

  default:
    /* Invalid values */
    msg68_warning("ym-2149: unknown ym-engine -- *%d*\n", engine);
  case YM_ENGINE_DEFAULT:
    /* Default values */
    engine = default_parms.engine;
  case YM_ENGINE_PULS:
  case YM_ENGINE_BLEP:
  case YM_ENGINE_DUMP:
    /* Valid values */
    if (!ym) {
      default_parms.engine = engine;
      msg68_notice("ym-2149: default engine -- *%s*\n",
                   ym_engine_name(engine));
    } else {
      ym->engine = engine;
    }
    break;
  }
  return engine;
}


/* ,-----------------------------------------------------------------.
 * |                           Master clock                          |
 * `-----------------------------------------------------------------'
 */

int ym_clock(ym_t * const ym, int clock)
{
  switch (clock) {

  case YM_CLOCK_QUERY:
    clock = ym ? ym->clock : default_parms.clock;
    break;

  case YM_CLOCK_DEFAULT:
    clock = default_parms.clock;

  default:
    if (clock != YM_CLOCK_ATARIST) {
      msg68_warning("ym-2149: unsupported clock -- *%u*\n",
                    (unsigned int) clock);
    }
    clock = YM_CLOCK_ATARIST;
    if (!ym) {
      default_parms.clock = clock;
      msg68_notice("ym-2149: default clock -- *%u*\n",
                   (unsigned int) clock);
    } else {
      clock = ym->clock;
    }
    break;
  }
  return clock;
}


/* ,-----------------------------------------------------------------.
 * |                           Volume model                          |
 * `-----------------------------------------------------------------'
 */

static
const char * ym_volmodel_name(int model)
{
  switch (model) {
  case YM_VOL_LINEAR:  return "linear";
  case YM_VOL_ATARIST: return "atari";
  }
  return 0;
}

int ym_volume_model(ym_t * const ym, int model)
{
  /* Set volume table (unique for all instance) */
  switch (model) {

  case YM_VOL_QUERY:
    model = default_parms.volmodel;
    break;

  default:
    msg68_warning("ym-2149: unknown volume model -- %d\n", model);
  case YM_VOL_DEFAULT:
    model = default_parms.volmodel;
  case YM_VOL_LINEAR:
  case YM_VOL_ATARIST:
    if (ym) {
      model = ym->volmodel;
    } else {
      ym->volmodel = model;
      assert(model == YM_VOL_LINEAR || model == YM_VOL_ATARIST);
      if ( model == YM_VOL_LINEAR ) {
        ym_create_5bit_linear_table(ymout5, output_level);
      } else {
        ym_create_5bit_atarist_table(ymout5, output_level);
      }
      msg68_notice("ym-2149: default volume model -- *%s*\n",
                 ym_volmodel_name(model));
    }
    break;
  }
  return model;
}


/* ,-----------------------------------------------------------------.
 * |                        Output sampling rate                     |
 * `-----------------------------------------------------------------'
 */

int ym_sampling_rate(ym_t * const ym, const int chz)
{
  int hz = chz;
  switch (hz) {

  case YM_VOL_QUERY:
    hz = ym ? ym->hz : default_parms.hz;
    break;

  case YM_VOL_DEFAULT:
    hz = default_parms.hz;

  default:
    if (hz < SAMPLING_RATE_MIN) hz = SAMPLING_RATE_MIN;
    if (hz > SAMPLING_RATE_MAX) hz = SAMPLING_RATE_MAX;
    if (ym->cb_sampling_rate) {
      /* engine sampling rate callback */
      hz = ym->cb_sampling_rate(ym,hz);
    }
    if (ym) {
      ym->hz = hz;
    } else {
      default_parms.hz = hz;
    }
    msg68_notice("ym-2149: %s sampling rate -- *%dhz*\n",
               ym ? "select" : "default", hz);
  }
  return hz;
}


/* ,-----------------------------------------------------------------.
 * |               Configure default or emulator instance            |
 * `-----------------------------------------------------------------'
 */

int ym_configure(ym_t * const ym, ym_parms_t * const parms)
{
  if (!parms) {
    msg68_error("ym-2149: nothing to configure\n");
    return -1;
  }
  parms->engine   = ym_engine(ym, parms->engine);
  parms->volmodel = ym_volume_model(ym, parms->volmodel);
  parms->clock    = ym_clock(ym, parms->clock);
  parms->hz       = ym_sampling_rate(ym, parms->hz);
  return 0;
}


/* ,-----------------------------------------------------------------.
 * |                  Setup / Cleanup emulator instance              |
 * `-----------------------------------------------------------------'
 */

int ym_setup(ym_t * const ym, ym_parms_t * const parms)
{
  ym_parms_t * const p = parms ? parms : &default_parms;
  int err = -1;

  /* engine */
  if (p->engine == YM_ENGINE_DEFAULT) {
    p->engine = default_parms.engine;
  }

  /* sampling rate */
  if (p->hz == 0) {
    p->hz = default_parms.hz;
  }

  /* clock */
  switch (p->clock) {
  case YM_CLOCK_ATARIST:
    break;
  case YM_CLOCK_DEFAULT:
  default:
    p->clock = default_parms.clock;
  }

  TRACE68(ym_cat,"ym-2149: setup -- engine:%d rate:%d clock:%d level:%d\n",
          p->engine,p->hz,p->clock,256);

  if (ym) {
    ym->overflow    = 0;
    ym->ymout5      = ymout5;
    ym->waccess_max = sizeof(ym->static_waccess)/sizeof(*ym->static_waccess);
    ym->waccess     = ym->static_waccess;
    ym->waccess_nxt = ym->waccess;
    ym->clock       = p->clock;
/*     ym->outlevel    = p->outlevel >> 2; */
    ym->voice_mute  = ym_smsk_table[7 & ym_default_chans];
    /* clearing sampling rate callback ensure requested rate to be in
       valid range. */
    ym->cb_sampling_rate = 0;
    ym_sampling_rate(ym, p->hz);
    ym->engine = p->engine;

    TRACE68(ym_cat,"ym-2149: engine -- *%d*\n", p->engine);

    switch (p->engine) {
    case YM_ENGINE_PULS:
      err = ym_puls_setup(ym);
      break;

    case YM_ENGINE_BLEP:
      err = ym_blep_setup(ym);
      break;

    case YM_ENGINE_DUMP:
      err = ym_dump_setup(ym);
      break;

    default:
      msg68_critical("ym-2149: engine %d -- *invalid*\n", p->engine);
      err = -1;
    }
    if (!err)
      msg68_notice("ym-2149: engine -- *%s*\n", ym_engine_name(ym->engine));


    /* at this point specific sampling rate callback can be call */
    ym_sampling_rate(ym, ym->hz);
  }

  /* Just for info print */
  ym_active_channels(ym,0,0);

  msg68(ym_cat,"ym-2149: trace level -- *active*\n");

  return err ? err : ym_reset(ym, 0);
}


/** Destroy an Yamaha-2149 emulator instance.
 */
void ym_cleanup(ym_t * const ym)
{
  TRACE68(ym_cat,"%s","ym-2149: cleanup\n");
  if (ym) {
    if (ym->overflow)
      msg68_critical("ym-2149: write access buffer overflow -- *%u*\n",
                     ym->overflow);
    if (ym->cb_cleanup)
      ym->cb_cleanup(ym);
  }
}

/** Get required output buffer size.
 */
uint68_t ym_buffersize(const ym_t * const ym, const cycle68_t ymcycles)
{
  return ym->cb_buffersize(ym,ymcycles);
}
