/* libnih
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2009 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef NIH_OPTION_H
#define NIH_OPTION_H

/**
 * Provides a flexible command-line option and arguments parser that
 * automatically provides built-in options for adjusting output verbosity
 * and provides --help output.
 *
 * Describe your commands options using an array of NihOption members
 * and pass this along with the argc and argv variables to
 * nih_option_parser().
 *
 * Options can be set from arguments with helper functions, nih_option_count()
 * and nih_option_int() are provided for you.
 *
 * Help and usage output can be customised with the nih_option_set_usage(),
 * nih_option_set_usage_stem(), nih_option_set_synopsis(),
 * nih_option_set_help() and nih_option_set_footer() functions.  Be sure
 * to call these before the option parser.
 **/

#include <nih/macros.h>


/**
 * NihOptionSetter;
 * @option: NihOption invoked,
 * @arg: argument to parse.
 *
 * An option setter is a function that is called whenever an option is
 * found in the command-line arguments.  If the option expects a value
 * then the argument is passed to the function, otherwise NULL is
 * passed.
 *
 * The function may use the value member of @option to store the parsed
 * value, or may take its own action.
 *
 * If there is an error with the argument the function may return zero
 * to abort parsing.
 **/
typedef struct nih_option NihOption;
typedef int (*NihOptionSetter) (NihOption *option, const char *arg);


/**
 * NihOptionGroup:
 * @title: descriptive help message.
 *
 * This structure is used to define a group of options that are collated
 * together when --help is given.
 **/
typedef struct nih_option_group {
	char *title;
} NihOptionGroup;

/**
 * NihOption:
 * @option: short option character,
 * @long_option: long option name,
 * @help: descriptive help message,
 * @group: group option is member of,
 * @arg_name: description of value or NULL if option takes no argument,
 * @value: pointer of variable to store argument in,
 * @setter: function to store the value.
 *
 * This structure defines an option that may be found in the command-line
 * arguments.  One or both of @option and @long_option myst be specified,
 * if no short option is wanted it may be zero and if no @long_option is
 * wanted it may be NULL.
 *
 * If @arg_name is not NULL then the option takes the next non-option
 * argument from the command-line; if @setter is not NULL it is passed
 * this argument, otherwise @value should contain the address of a char *
 * in which a newly allocated copy of the argument will be stored.
 *
 * If @arg_name is NULL then NULL if passed to @setter.  If both
 * @arg_name and @setter are NULL then @value should contain the address
 * of an int in which a TRUE value is stored.
 **/
struct nih_option {
	int              option;
	char            *long_option;

	char            *help;
	NihOptionGroup  *group;

	char            *arg_name;
	void            *value;

	NihOptionSetter  setter;
};


/**
 * NIH_OPTION_LAST:
 *
 * This macro may be used as the last option in the list to avoid typing
 * all those NULLs and 0s yourself.
 **/
#define NIH_OPTION_LAST { 0, NULL, NULL, NULL, NULL, NULL, NULL }


NIH_BEGIN_EXTERN

char **    nih_option_parser         (const void *parent,
				      int argc, char *argv[],
				      NihOption *options, int break_nonopt)
	__attribute__ ((warn_unused_result, malloc));

NihOption *nih_option_join           (const void *parent,
				      const NihOption *a, const NihOption *b)
	__attribute__ ((warn_unused_result, malloc));

int        nih_option_count          (NihOption *option, const char *arg);
int        nih_option_int            (NihOption *option, const char *arg);

int        nih_option_quiet          (NihOption *option, const char *arg);
int        nih_option_verbose        (NihOption *option, const char *arg);
int        nih_option_debug          (NihOption *option, const char *arg);

void       nih_option_set_usage      (const char *usage);
void       nih_option_set_usage_stem (const char *usage);
void       nih_option_set_synopsis   (const char *synopsis);
void       nih_option_set_help       (const char *help);
void       nih_option_set_footer     (const char *footer);

NIH_END_EXTERN

#endif /* NIH_OPTION_H */
