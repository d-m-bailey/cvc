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

#ifndef _MMAP_FILE_POOL_H
#define _MMAP_FILE_POOL_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <map>
#include "mmap_access_mode.h"

namespace mmap_allocator_namespace {
	class mmap_file_identifier {
public:
		mmap_file_identifier();
		mmap_file_identifier(const mmap_file_identifier &other);
		mmap_file_identifier(std::string fname, enum access_mode access_mode);
		bool operator==(const mmap_file_identifier &other) const;
		bool operator<(const mmap_file_identifier &other) const;

private:
		enum access_mode access_mode;
		dev_t device;
		ino_t inode;	
	};


	class mmapped_file {
public:
		mmapped_file():
			fd(-1),
			memory_area(NULL),
			size_mapped(0),
			offset_mapped(0),
			reference_count(0)
		{ }

		void *get_memory_area(void)
		{
			return memory_area;
		}

		void remmap_file_for_read();
		void* open_and_mmap_file(std::string fname, enum access_mode access_mode, off_t offset, size_t length, bool map_whole_file, bool allow_remap);
		bool munmap_and_close_file(void);

private:
		friend class mmap_file_pool;

		int fd;
		void *memory_area;
		size_t size_mapped;
		off_t offset_mapped;
		int reference_count;
	};

	typedef std::map<mmap_file_identifier, mmapped_file> mmapped_file_map_t;
	typedef std::pair<mmap_file_identifier, mmapped_file> mmapped_file_pair_t;

/* Singleton */
	class mmap_file_pool {
public:
		mmap_file_pool():
			the_map()
		{ }

		void *mmap_file(std::string fname, enum access_mode access_mode, off_t offset, size_t length, bool map_whole_file, bool allow_remap);
		void munmap_file(std::string fname, enum access_mode access_mode, off_t offset, size_t length);

private:
		mmapped_file_map_t the_map;
	};

	extern mmap_file_pool the_pool;
}

#endif
