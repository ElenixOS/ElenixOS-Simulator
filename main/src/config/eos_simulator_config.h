/**
 * @file eos_simulator_config.h
 * @brief Simulator platform configuration fallbacks
 */

#ifndef EOS_SIMULATOR_CONFIG_H
#define EOS_SIMULATOR_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Platform port fallbacks -----------------------------------*/

#ifndef EOS_PORT_MACOS_VOLUME_CONTROL_ENABLE
#define EOS_PORT_MACOS_VOLUME_CONTROL_ENABLE 1
#endif

#ifdef __cplusplus
}
#endif

#endif /* EOS_SIMULATOR_CONFIG_H */
