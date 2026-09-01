/**
 * @file eos_esh_vscode.h
 * @brief VSCode terminal frontend for ESH
 */

#ifndef EOS_ESH_VSCODE_H
#define EOS_ESH_VSCODE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ---------------------------------------------------*/
#include "eos_error.h"

/* Public function prototypes ---------------------------------*/

/**
 * @brief Initialize the simulator's stdin/stdout ESH frontend
 * @return EOS_OK on success
 */
eos_result_t eos_esh_vscode_init(void);

/**
 * @brief Poll stdin and push available bytes into ESH
 */
void eos_esh_vscode_poll(void);


#ifdef __cplusplus
}
#endif

#endif /* EOS_ESH_VSCODE_H */
