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

#ifndef NIH_TEST_H
#define NIH_TEST_H

/* For _GNU_SOURCE */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#define NIH_IN_TEST_H

#include <nih/test_output.h>
#include <nih/test_values.h>
#include <nih/test_process.h>
#include <nih/test_divert.h>
#include <nih/test_files.h>
#include <nih/test_alloc.h>
#include <nih/test_list.h>
#include <nih/test_hash.h>

#undef NIH_IN_TEST_H

#endif /* NIH_TEST_H */
