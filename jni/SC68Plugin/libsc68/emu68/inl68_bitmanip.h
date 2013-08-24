/**
 * @ingroup   emu68_lib_inline
 * @file      emu68/inl68_bitmanip.h
 * @brief     68k bit manipalution instruction inlines.
 * @author    Benjamin Gerard
 * @date      2009/05/18
 */
/* Time-stamp: <2013-08-04 23:11:36 ben>  */

/* Copyright (C) 1998-2013 Benjamin Gerard */

#ifndef INL68_BITMANIP_H
#define INL68_BITMANIP_H

static inline
void inl_btst68(emu68_t * const emu68, const int68_t v, const int bit)
{
  REG68.sr = ( REG68.sr & ~SR_Z )
    | ( ( (~v >> bit) & 1 ) << SR_Z_BIT );
}

static inline
int68_t inl_bset68(emu68_t * const emu68, const int68_t v, const int bit)
{
  inl_btst68(emu68, v, bit);
  return v | ( 1 << bit );
}

static inline
int68_t inl_bclr68(emu68_t * const emu68, const int68_t v, const int bit)
{
  inl_btst68(emu68, v, bit);
  return v & ~( 1 << bit );
}

static inline
int68_t inl_bchg68(emu68_t * const emu68, const int68_t v, const int bit)
{
  inl_btst68(emu68, v, bit);
  return v ^ ( 1 << bit );
}

#endif
