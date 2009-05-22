/* libnih
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2009 Canonical Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef NIH_COMMAND_H
#define NIH_COMMAND_H

/**
 * Builds on the command-line option and arguments parser to provide an
 * application interface where the first non-option argument is the name
 * of a command.  Both global and command-specific options are permitted,
 * and global options may appear both before and after the command.
 *
 * Description your commands using an array of NihCommand members,
 * with each describing its options using an array of NihOption members.
 * Pass this all to nih_command_parser().
 *
 * Commands are implemented with a handler function that is called,
 * when nih_command_parser returns it has completed its work.
 **/

#include <nih/macros.h>

#include <nih/option.h>


/**
 * NihCommandAction;
 * @command: NihCommand invoked,
 * @args: command-line arguments.
 *
 * A command action is a function called when a command is found in the
 * command-line arguments.  It is passed the list of arguments that
 * follow as an array allocated with nih_alloc().
 *
 * The return value of the function is returned from nih_command_parser().
 **/
typedef struct nih_command NihCommand;
typedef int (*NihCommandAction) (NihCommand *command, char * const *args);


/**
 * NihCommandGroup:
 * @title: descriptive help message.
 *
 * This structure is used to define a group of commands that are collated
 * together when help is given.
 **/
typedef struct nih_command_group {
	char *title;
} NihCommandGroup;

/**
 * NihCommand:
 * @command: command name,
 * @usage: usage string,
 * @synopsis: synopsis string,
 * @help: help string,
 * @group: group option is member of,
 * @options: command-specific options,
 * @action: function to call when found.
 *
 * This structure defines a command that may be found in the command-line
 * arguments after any application-specific options, and before any
 * command-specific options.  @command must be specified which is the
 * string looked for.
 *
 * After @command is found in the arguments, following options are
 * considered specific to the command.  These are specified in @options,
 * which should be an array of NihOption structures terminated by
 * NIH_OPTION_LAST.  This may be NULL, in which case it is treated the same
 * as an empty list.
 *
 * Any remaining command-line arguments are placed in an array and given
 * as an argument to the @action function.
 *
 * Help for the command is built from @usage, @synopsis and @help as if
 * they were passed to nih_option_set_usage(), etc. for this command.
 **/
struct nih_command {
	char             *command;

	char             *usage;
	char             *synopsis;
	char             *help;

	NihCommandGroup  *group;

	NihOption        *options;

	NihCommandAction  action;
};


/**
 * NIH_COMMAND_LAST:
 *
 * This macro may be used as the last command in the list to avoid typing
 * all those NULLs yourself.
 **/
#define NIH_COMMAND_LAST { NULL, NULL, NULL, NULL, NULL, NULL, NULL }


NIH_BEGIN_EXTERN

int         nih_command_parser (const void *parent, int argc, char *argv[],
				NihOption *options, NihCommand *commands);

NihCommand *nih_command_join   (const void *parent,
				const NihCommand *a, const NihCommand *b)
	__attribute__ ((warn_unused_result, malloc));

NIH_END_EXTERN

#endif /* NIH_COMMAND_H */
