/*
 * This code is (c) 2012 Johannes Thoma
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * You can download original source from https://github.com/johannesthoma/mmap_allocator.git
 */

#ifndef _MMAP_ACCESS_MODE
#define _MMAP_ACCESS_MODE

#include <stdio.h>

#define ALIGN_TO_PAGE(x) ((x) & ~(getpagesize() - 1))
#define UPPER_ALIGN_TO_PAGE(x) ALIGN_TO_PAGE((x)+(getpagesize()-1))
#define OFFSET_INTO_PAGE(x) ((x) & (getpagesize() - 1))

namespace mmap_allocator_namespace
{
	enum access_mode {
		DEFAULT_STL_ALLOCATOR, /* Default STL allocator (malloc based). Reason is to have containers that do both and are compatible */
		READ_ONLY,  /* Readonly modus. Segfaults when vector content is written to */
		READ_WRITE_PRIVATE, /* Read/write access, writes are not propagated to disk */
		READ_WRITE_SHARED  /* Read/write access, writes are propagated to disk (file is modified) */
	};

	enum allocator_flags {
		MAP_WHOLE_FILE = 1,
		ALLOW_REMAP = 2,
		BYPASS_FILE_POOL = 4,
		KEEP_FOREVER = 8
	};

	void set_verbosity(int v);
	int get_verbosity(void);
}

#endif
