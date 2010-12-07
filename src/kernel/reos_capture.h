#ifndef REOS_CAPTURE_H
#define REOS_CAPTURE_H

#include "reos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

ReOS_CaptureSet *new_reos_captureset(ReOS_CompoundList *);
void free_reos_captureset(ReOS_CaptureSet *);
void delete_reos_captureset(ReOS_CaptureSet *);

ReOS_CaptureSet *reos_captureset_detach(ReOS_CaptureSet *);
void reos_captureset_ref(ReOS_CaptureSet *);
void reos_captureset_deref(ReOS_CaptureSet *);

void reos_captureset_save_start(ReOS_CaptureSet **, int, long);
void reos_captureset_save_end(ReOS_CaptureSet **, int, long);
ReOS_Capture *reos_captureset_get_capture(ReOS_CaptureSet *, int);

long reos_captureset_reconstruct(ReOS_CaptureSet *, ReOS_Input *, void *);
long reos_capture_reconstruct(ReOS_Capture *, ReOS_Input *, void *);

ReOS_BackrefBuffer *new_reos_backrefbuffer(ReOS_Capture *, ReOS_Input *);
void free_reos_backrefbuffer(ReOS_BackrefBuffer *);

#ifdef __cplusplus
}
#endif

#endif
