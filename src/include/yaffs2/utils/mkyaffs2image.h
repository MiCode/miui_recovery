#ifndef MKYAFFS2IMAGE_H
#define MKYAFFS2IMAGE_H

typedef void (*mkyaffs2image_callback) (char* filename);

int mkyaffs2image(char* target_directory, char* filename, int fixstats, mkyaffs2image_callback callback);

#endif
