/**
 * @ingroup   sc68_lib
 * @file      conf68.h
 * @brief     configuration file.
 * @author    Benjamin Gerard
 * @date      1999/07/27
 */
/* Time-stamp: <2013-08-07 19:59:02 ben> */

/* Copyright (C) 1998-2013 Benjamin Gerard */

#ifndef SC68_CONF68_H
#define SC68_CONF68_H

#ifndef SC68_EXTERN
# ifdef __cplusplus
#  define SC68_EXTERN extern "C"
# else
#  define SC68_EXTERN
# endif
#endif

/**
 *  @defgroup  sc68_conf  configuration file
 *  @ingroup   sc68_lib
 *
 *  This module prodives functions to access sc68 configuration file.
 *
 *  @{
 */

/**
 * Config entry types.
 */
enum _config68_type_e {
  CONFIG68_UND=0,  /**< Value is not set.     */
  CONFIG68_INT,    /**< Value is an integer.  */
  CONFIG68_STR,    /**< Value is a string.    */
  CONFIG68_ERR=-1  /**< Reserved for errors.  */
};

typedef enum _config68_type_e config68_type_t;

/**
 * Config type.
 */
typedef struct _config68_s config68_t;

SC68_EXTERN
/**
 *  Check if the config values are valid.
 *
 *  @param   conf  config instance
 *
 *  @return   0 config is valid
 *  @retval  -1 on error (config is not valid)
 *
 */
int config68_valid(config68_t * conf);

/**
 *  Get index of named config entry.
 *
 *  @param   conf  config instance
 *  @param   name  name of entry
 *
 *  @return  index
 *  @retval  -1 on error
 */
int config68_get_idx(const config68_t * conf,
                     const char * name);

SC68_EXTERN
/**
 *  Get type and range of a config entry.
 *
 *  @param  conf  config instance
 *  @param  idx   index
 *  @param  min   store min value (0 ignore)
 *  @param  max   store max value (0 ignore)
 *  @param  def   store default value (0 ignore)
 *
 *  @return  type of config entry
 *  @retval  CONFIG68_ERROR on error
 */
config68_type_t config68_range(const config68_t * conf, int idx,
                               int * min, int * max, int * def);

SC68_EXTERN
/**
 *  Get value of a config entry.
 *
 *  @param  conf  config instance
 *  @param  v     input: pointer to index; output: integer value or unmodified
 *  @param  name  input: pointer to name; output: string value or unmodified
 *
 *  @return  type of config entry
 *  @retval  CONFIG68_ERROR on error
 */
config68_type_t config68_get(const config68_t * conf,
                             int * v,
                             const char ** name);

SC68_EXTERN
/**
 *  Set value of a config entry.
 *
 *  @param  conf  config.
 *  @param  idx   index of config entry used only if name not found.
 *  @param  name  pointer to name (0:use idx).
 *  @param  v     value used to set CONFIG68_INT entry.
 *  @param  s     value used to set CONFIG68_STRING entry.
 *
 *  @return type of config entry
 *  @retval CONFIG68_ERROR on error
 */
config68_type_t config68_set(config68_t * conf,
                             int idx,
                             const char * name,
                             int v,
                             const char * s);

SC68_EXTERN
/**
 *  Load config from file.
 *
 *  @param  conf  config instance
 *
 *  @return error code
 *  @retval 0 on success
 *  @retval 0 on error
 */
int config68_load(config68_t * conf);

SC68_EXTERN
/**
 *  Save config into file.
 *
 *  @param  conf  config
 *
 *  @return error code
 *  @retval 0 on success
 *  @retval 0 on error
 */
int config68_save(config68_t * conf);

SC68_EXTERN
/**
 *  Fill config struct with default value.
 *
 *  @param  conf  config
 *
 *  @return error code
 *  @retval 0 on success
 *  @retval 0 on error
 */
int config68_default(config68_t * conf);

SC68_EXTERN
/**
 *  Create config.
 *
 *  @param  name  config name (0:default)
 *  @param  size  number of config entries to prealloc
 *
 *  @return config instance
 *  @retval 0 on error
 */
config68_t * config68_create(const char * name, int size);

SC68_EXTERN
/**
 *  Destroy config.
 *
 *  @param  conf  config instance
 */
void config68_destroy(config68_t * conf);

SC68_EXTERN
/**
 *  Library one time init.
 *
 *  @param  force_file  Set to force the use of config file over registry.
 *
 *  @return error code
 *  @retval 0 on success
 *  @retval 0 on error
 */
int config68_init(int force_file);

SC68_EXTERN
/**
 *  Library one time shutdown shutdown.
 */
void config68_shutdown(void);

/**
 * @}
 */

#endif
