#ifndef _CONCAT_H_
#define _CONCAT_H_

/* note that all functions in this class are creating copies of the data, which
   means this is damns slow and memory wasting, but ensures the memory does not
   get corrupted from outside!
*/

#include "libeptool.h"

typedef struct _t_concat_buffer *HANDLE_CONCAT_BUFFER;

/* --- concatenation buffer --- */

HANDLE_CONCAT_BUFFER concatenation_create(EPConfig *epcfg);
void concatenation_free(HANDLE_CONCAT_BUFFER concat_buffer);

/* --- encoding --- */

/*
  add a frame to concatenation buffer
    if successful, return number of frames still needed
                   i.e. return 0 on last frame
    if buffer is full, return -2
 */
int concatenation_add(
	int choiceOfPred,
	HANDLE_CONCAT_BUFFER concat_buffer,
	const HANDLE_CLASS_FRAME inFrame);

/*
  get the number of "classes" after encoding the buffer
    if successful, return number
    if buffer not yet full, return -2
 */
int concatenation_classes_after_concat(
	const HANDLE_CONCAT_BUFFER concat_buffer);

/*
  encode the concatenation buffer to a frame
  the concatenated classes with their real class id is stored in the arrays
    if successful, return number of output classes
    if buffer not yet full, return -2
 */
int concatenation_concat_buffer(
	HANDLE_CONCAT_BUFFER concat_buffer,
	unsigned char **out_classes,
	long *out_len,
	int *out_class_id);

/* --- decoding --- */

/*
  tell the epTool, how many classes are to be found in his input
*/
int concatenation_classes_in_frame(
	const HANDLE_CONCAT_BUFFER concat_buffer,
	int pred_set);

/*
  tell the epTool, which classes are where to be found in his input
    return the number of class instances in the ep-frame
*/
int concatenation_class_order_in_frame(
	const HANDLE_CONCAT_BUFFER concat_buffer,
	int pred_set,
	int *out_class_ids);

/*
  demultiplex a concatenated frame, fill the buffer
    return number of frames in concatenation
    if buffer was not yet empty, do anyway and return -2
*/
int concatenation_demux(
	HANDLE_CONCAT_BUFFER concat_buffer,
	int pred_set,
	const unsigned char **class_data,
	const long *lengths);

/*
  retrieve the next frame from the buffer
    if successful, return number of frames still in buffer
                   i.e. return 0 if buffer is empty
    if buffer was already empty, return -2
*/
int concatenation_retrieve(
	HANDLE_CONCAT_BUFFER concat_buffer,
	HANDLE_CLASS_FRAME outFrame);

#endif
