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
 */

#ifndef _MMAP_EXCEPTION
#define _MMAP_EXCEPTION

#include "mmap_access_mode.h"
#include <stdexcept>

namespace mmap_allocator_namespace
{
	class mmap_allocator_exception: public std::runtime_error {
public:
		mmap_allocator_exception(const char *msg_param) throw(): 
			std::runtime_error(msg_param)
		{
			if (get_verbosity() > 0) {
				fprintf(stderr, "Throwing exception %s\n", msg_param);
			}
		}

		mmap_allocator_exception(std::string msg_param) throw(): 
			std::runtime_error(msg_param)
		{
			if (get_verbosity() > 0) {
				fprintf(stderr, "Throwing exception %s\n", msg_param.c_str());
			}
		}

		virtual ~mmap_allocator_exception(void) throw()
		{
		}
private:
	};
}

#endif
