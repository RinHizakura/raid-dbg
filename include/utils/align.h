#ifndef ALIGN_H
#define ALIGN_H

/* reference:
 * - https://github.com/torvalds/linux/blob/master/include/linux/align.h */

#define __ALIGN(x, a) __ALIGN_MASK(x, (typeof(x))(a) -1)
#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define ALIGN_UP(x, a) __ALIGN((x), (a))
#define ALIGN_DOWN(x, a) __ALIGN((x) - ((a) -1), (a))

#endif
