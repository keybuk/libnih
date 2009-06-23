/* libnih
 *
 * command.c - command parser based on nih_option_parser
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <stdio.h>
#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/main.h>
#include <nih/option.h>
#include <nih/command.h>
#include <nih/logging.h>


/* Prototypes for static functions */
static NihCommand *nih_command_get        (NihCommand *commands,
				           const char *command);
static int         nih_command_handle     (const void *parent,
				           int argc, char *argv[],
				           NihOption *options,
				           NihCommand *commands,
				           NihCommand *command);
static void        nih_command_help       (NihCommand *commands);
static void        nih_command_group_help (NihCommandGroup *group,
					   NihCommand *commands,
					   NihCommandGroup **groups);


/**
 * default_commands:
 *
 * These default commands are appended to those defined by the user
 * so they can be overriden.
 **/
static const NihCommand default_commands[] = {
	{ "help", NULL,
	  N_("display list of commands"), NULL, NULL, NULL, NULL },

	NIH_COMMAND_LAST
};

/**
 * no_options:
 *
 * This is used whenever the options member of NihCommand is NULL.
 **/
static const NihOption no_options[] = {
	NIH_OPTION_LAST
};


/**
 * nih_command_parser:
 * @parent: parent for arguments arrays,
 * @argc: number of arguments,
 * @argv: command-line arguments,
 * @options: global options,
 * @commands: commands to look for.
 *
 * Parses the command-line arguments given in @argv until the first
 * non-option argument is found.  Options preceeding that are handled
 * according to @options by nih_option_parser().
 *
 * The argument is looked up in @commands, and if found, that is used to
 * process the remaining options and arguments.
 *
 * Alternatively if the program name can be found in @commands, then the
 * entire @argv list is treated as the command instead of locating the
 * first non-option.
 *
 * Reminaing arguments are passed to the action function of the @commands
 * member found.
 *
 * The usage stem and string are constructed automatically, calling
 * nih_option_set_usage() or nih_option_set_usage_stem() before this
 * function will have no effect.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for arguments arrays.  When all parents
 * of the array are freed, the array will also be freed.
 *
 * Errors are handled by printing a message to standard error.
 *
 * Returns: return value from action function or negative value on error.
 **/
int
nih_command_parser (const void *parent,
		    int         argc,
		    char       *argv[],
		    NihOption  *options,
		    NihCommand *commands)
{
	nih_local NihCommand  *cmds = NULL;
	NihCommand            *cmd;
	nih_local char        *footer = NULL, *stem = NULL;
	nih_local char       **args = NULL;
	char                 **arg;
	int          ret;

	nih_assert (argc > 0);
	nih_assert (argv != NULL);
	nih_assert (options != NULL);
	nih_assert (commands != NULL);
	nih_assert (program_name != NULL);

	cmds = nih_command_join (NULL, commands, default_commands);

	/* First check the program name for a valid command */
	cmd = nih_command_get (cmds, program_name);
	if (cmd)
		return nih_command_handle (parent, argc, argv,
					   options, cmds, cmd);

	/* Set help strings to make ordinary --help look right */
	footer = NIH_MUST (nih_sprintf (NULL, _("For a list of commands, "
						"try `%s help'."),
					program_name));
	nih_option_set_footer (footer);
	nih_option_set_usage (_("COMMAND [OPTION]... [ARG]..."));

	/* Parse options up until the first non-opt argument */
	args = nih_option_parser (NULL, argc, argv, options, TRUE);

	/* Clean up help strings */
	nih_option_set_footer (NULL);
	nih_option_set_usage (NULL);

	/* Check for option parsing errors */
	if (! args)
		return -1;

	/* Check we actually got a command */
	if (! args[0]) {
		fprintf (stderr, _("%s: missing command\n"), program_name);
		nih_main_suggest_help ();
		return -1;
	}

	/* Find that command */
	cmd = nih_command_get (cmds, args[0]);
	if (! cmd) {
		fprintf (stderr, _("%s: invalid command: %s\n"),
			 program_name, args[0]);
		nih_main_suggest_help ();
		return -1;
	}

	/* Count the number of arguments in the args array */
	for (arg = args; *arg; arg++)
		;

	/* Set the usage stem to include the command name */
	stem = NIH_MUST (nih_sprintf (NULL, _("%s [OPTION]..."),
				      cmd->command));
	nih_option_set_usage_stem (stem);

	/* Handle the command */
	ret = nih_command_handle (parent, arg - args, args,
				  options, cmds, cmd);

	/* Clean up usage stem */
	nih_option_set_usage_stem (NULL);

	return ret;
}


/**
 * nih_command_join:
 * @parent: parent object for new array,
 * @a: first command array,
 * @b: second command array.
 *
 * Joins the two command arrays together to produce a combined array
 * containing the commands from @a followed by the commands from @b.
 *
 * The new list is allocated with nih_alloc(), but the members are just
 * copied in from @a and @b including any pointers therein.  Freeing the
 * new array with nih_free() is entirely safe.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned array.  When all parents
 * of the returned array are freed, the returned array will also be
 * freed.
 *
 * Returns: combined command array.
 **/
NihCommand *
nih_command_join (const void       *parent,
		  const NihCommand *a,
		  const NihCommand *b)
{
	const NihCommand *cmd;
	NihCommand       *cmds;
	size_t            alen = 0, blen = 0;

	nih_assert (a != NULL);
	nih_assert (b != NULL);

	/* Count commands in first list */
	for (cmd = a; cmd->command; cmd++)
		alen++;

	/* Count commands in second list */
	for (cmd = b; cmd->command; cmd++)
		blen++;

	/* Allocate combined list */
	cmds = NIH_MUST (nih_alloc (parent,
				    sizeof (NihCommand) * (alen + blen + 1)));

	/* Copy options, making sure to copy the last option from b */
	memcpy (cmds, a, sizeof (NihCommand) * alen);
	memcpy (cmds + alen, b, sizeof (NihCommand) * (blen + 1));

	return cmds;
}

/**
 * nih_command_get:
 * @commands: command list,
 * @command: command to find.
 *
 * Find the command structure with the given @command in the @commands list.
 *
 * Returns; pointer to command or NULL if not found.
 **/
static NihCommand *
nih_command_get (NihCommand *commands,
		 const char *command)
{
	NihCommand *cmd;

	for (cmd = commands; cmd->command; cmd++)
		if (! strcmp (command, cmd->command))
			return cmd;

	return NULL;
}


/**
 * nih_command_handle:
 * @parent: parent for arguments arrays,
 * @argc: number of arguments,
 * @argv: command-line arguments,
 * @options: global options,
 * @commands: commands looked for,
 * @command: NihCommand invoked.
 *
 * This function is called to handle a @command that was either invoked
 * directly by program name, or found as an argument on the command line.
 * The list of commands looked for should be in @commands so that the
 * "help" command can be handled.
 *
 * @argv should be the list of arguments starting from the name of the
 * command, which is skipped.  @options is added to any options specified
 * in @command so that global options are always available.
 *
 * After parsing the options, remaining arguments are passed to the action
 * function of @command.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the arguments arrays.  When all parents
 * of the array are freed, the array will also be freed.
 *
 * Errors are handled by printing a message to standard error.
 *
 * Returns: return value from action or negative value on error.
 **/
static int
nih_command_handle (const void *parent,
		    int         argc,
		    char       *argv[],
		    NihOption  *options,
		    NihCommand *commands,
		    NihCommand *command)
{
	char                **args;
	const NihOption      *cmd_opts;
	nih_local NihOption  *opts = NULL;
	int                   ret;

	nih_assert (argc > 0);
	nih_assert (argv != NULL);
	nih_assert (options != NULL);
	nih_assert (commands != NULL);
	nih_assert (command != NULL);

	/* Join the command and global options together, allow command to
	 * take precedence
	 */
	cmd_opts = command->options ? command->options : no_options;
	opts = nih_option_join (NULL, cmd_opts, options);

	/* Set up the option parser from the command information */
	nih_option_set_usage (_(command->usage));
	nih_option_set_synopsis (_(command->synopsis));
	nih_option_set_help (_(command->help));

	/* Parse the remaining arguments against all of the options */
	args = nih_option_parser (parent, argc, argv, opts, FALSE);

	/* Clean up help strings again */
	nih_option_set_usage (NULL);
	nih_option_set_synopsis (NULL);
	nih_option_set_help (NULL);

	/* Check for option parsing failure */
	if (! args)
		return -1;

	/* Handle the special cased commands first */
	if (! strcmp (command->command, "help")) {
		nih_command_help (commands);
		exit (0);
	}

	/* Delegate to the command handler */
	ret = command->action (command, args);

	nih_free (args);
	return ret;
}


/**
 * nih_command_help:
 * @commands: list of commands.
 *
 * Output a list of the known commands to standard output grouped by the
 * group member of the command.
 **/
static void
nih_command_help (NihCommand  *commands)
{
	NihCommand                 *cmd;
	nih_local NihCommandGroup **groups = NULL;
	size_t                      group, ngroups;
	int                         other = FALSE;

	nih_assert (program_name != NULL);

	groups = NULL;
	ngroups = 0;

	/* Count the number of command groups */
	for (cmd = commands; cmd->command; cmd++) {
		if (! cmd->group) {
			other = TRUE;
			continue;
		}

		for (group = 0; group < ngroups; group++) {
			if (groups[group] == cmd->group)
				break;
		}

		if (group < ngroups)
			continue;

		groups = NIH_MUST (nih_realloc (groups, NULL,
						    (sizeof (NihCommandGroup *)
						     * (ngroups + 1))));
		groups[ngroups++] = cmd->group;
	}

	/* Iterate the command groups we found in order, and display
	 * only their commands
	 */
	for (group = 0; group < ngroups; group++)
		nih_command_group_help (groups[group], commands, groups);

	/* Display the other group */
	if (other)
		nih_command_group_help (NULL, commands, groups);

	/* Say how to find out about a command */
	printf (_("For more information on a command, "
		  "try `%s COMMAND --help'.\n"), program_name);
}

/**
 * nih_command_group_help:
 * @group: group to display,
 * @commands: list of commands,
 * @groups: all groups.
 *
 * Output a list of commands in the given @group to standard output.
 **/
static void
nih_command_group_help (NihCommandGroup  *group,
			NihCommand       *commands,
			NihCommandGroup **groups)
{
	NihCommand *cmd;
	size_t      width;

	nih_assert (commands != NULL);

	if (group) {
		printf (_("%s commands:\n"), _(group->title));
	} else if (groups) {
		printf (_("Other commands:\n"));
	} else {
		printf (_("Commands:\n"));
	}

	width = nih_max (nih_str_screen_width (), 50U) - 30;

	for (cmd = commands; cmd->command; cmd++) {
		nih_local char *str = NULL;
		char           *ptr;
		size_t          len = 0;

		if (cmd->group != group)
			continue;

		if (! cmd->synopsis)
			continue;

		/* Indent by two spaces */
		printf ("  ");
		len += 2;

		/* Output the command */
		printf ("%s", cmd->command);
		len += strlen (cmd->command);

		/* Format the synopsis string to fit in the latter
		 * half of the screen
		 */
		str = NIH_MUST (nih_str_wrap (NULL, cmd->synopsis,
					      width, 0, 2));

		/* Write the description to the screen */
		ptr = str;
		while (ptr && *ptr) {
			size_t linelen;

			/* Not enough room on this line */
			if (len > 28) {
				printf ("\n");
				len = 0;
			}

			/* Pad line up to the right column */
			while (len < 30) {
				printf (" ");
				len++;
			}

			/* Output the line up until the next line */
			linelen = strcspn (ptr, "\n");
			printf ("%.*s\n", (int)linelen, ptr);
			len = 0;

			/* Skip to the next line */
			ptr += linelen;
			if (*ptr == '\n')
				ptr++;
		}
	}

	printf ("\n");
}
