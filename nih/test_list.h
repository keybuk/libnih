/* libnih
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef NIH_TEST_LIST_H
#define NIH_TEST_LIST_H

#ifndef NIH_IN_TEST_H
# error "This header may only be included by <nih/test.h>"
#endif /* NIH_IN_TEST_H */

#include <nih/list.h>


/**
 * TEST_LIST_EMPTY:
 * @_list: entry in list.
 *
 * Check that the list of which @_list is a member is empty, ie. that
 * @_list is the sole member.
 **/
#define TEST_LIST_EMPTY(_list) \
	if (! NIH_LIST_EMPTY (_list)) \
		TEST_FAILED ("list %p (%s) not empty as expected", \
			     (_list), #_list)

/**
 * TEST_LIST_NOT_EMPTY:
 * @_list: entry in list.
 *
 * Check that the list of which @_list is a member is not empty, ie. that
 * there are more members than just @_list.
 **/
#define TEST_LIST_NOT_EMPTY(_list) \
	if (NIH_LIST_EMPTY (_list)) \
		TEST_FAILED ("list %p (%s) empty, expected multiple members", \
			     (_list), #_list)

#endif /* NIH_TEST_LIST_H */
