/* libnih
 *
 * option.c - command-line argument and option parsing
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


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/main.h>
#include <nih/option.h>
#include <nih/logging.h>


/**
 * NihOptionCtx:
 * @parent: parent object for returned array,
 * @argc: number of arguments,
 * @argv: arguments array,
 * @options: list of options,
 * @nargs: number of non-option arguments,
 * @args: non-option arguments to return,
 * @arg: current argument index,
 * @nonopt: index of first non-option argument,
 * @optend: index of end of options.
 *
 * This structure is passed between the various parsing functions to avoid
 * having to pass a dozen different variables around and to keep us
 * re-entrant.
 **/
typedef struct nih_option_ctx {
	const void  *parent;
	int          argc;
	char       **argv;
	NihOption   *options;

	size_t       nargs;
	char       **args;

	int          arg;
	int          nonopt;
	int          optend;
} NihOptionCtx;


/* Prototypes for static functions */
static NihOption * nih_option_get_short   (NihOptionCtx *ctx, int option);
static int         nih_option_short       (NihOptionCtx *ctx);
static NihOption * nih_option_get_long    (NihOptionCtx *ctx,
					   const char *option, size_t len);
static int         nih_option_long        (NihOptionCtx *ctx);
static int         nih_option_handle      (NihOptionCtx *ctx, NihOption *opt);
static int         nih_option_handle_arg  (NihOptionCtx *ctx, NihOption *opt,
					   const char *arg);
static const char *nih_option_next_nonopt (NihOptionCtx *ctx);
static void        nih_option_help        (NihOption *options);
static void        nih_option_group_help  (NihOptionGroup *group,
					   NihOption *options,
					   NihOptionGroup **groups);


/**
 * default_options:
 *
 * These default options are appended to those defined by the user
 * so they can be overriden.
 **/
static const NihOption default_options[] = {
	{ 'q', "quiet",
	  N_("reduce output to errors only"),
	  NULL, NULL, NULL, nih_option_quiet },
	{ 'v', "verbose",
	  N_("increase output to include informational messages"),
	  NULL, NULL, NULL, nih_option_verbose },
	/* Deliberately hidden, you get told about this when you file
	 * a bug ;-)
	 */
	{ 0, "debug", NULL, NULL, NULL, NULL, nih_option_debug },
	{ 0, "help",
	  N_("display this help and exit"),
	  NULL, NULL, NULL, NULL },
	{ 0, "version",
	  N_("output version information and exit"),
	  NULL, NULL, NULL, NULL },

	NIH_OPTION_LAST
};

/**
 * usage_stem:
 *
 * This string is prepended to the program's usage line if given.
 **/
static const char *usage_stem = NULL;

/**
 * usage_string:
 *
 * This string is appended to the program's usage line if given.
 **/
static const char *usage_string = NULL;

/**
 * synopsis_string:
 *
 * This string is output after the program's usage string if given.
 **/
static const char *synopsis_string = NULL;

/**
 * help_string:
 *
 * This string is output after the options if given.
 **/
static const char *help_string = NULL;

/**
 * footer_string:
 *
 * This string is output after the options and help if given.
 **/
static const char *footer_string = NULL;


/**
 * nih_option_parser:
 * @parent: parent object for returned array,
 * @argc: number of arguments,
 * @argv: command-line arguments,
 * @options: options to look for,
 * @break_nonopt: stop processing options at first non-option argument.
 *
 * Parses the command-line arguments given in @argv looking for options
 * described in @options, or those built-in.  Options are handled according
 * to common UNIX semantics so that short options may be grouped together
 * and arguments need not immediately follow the option that requires it.
 *
 * Remaining non-option arguments are placed into an array for processing
 * by the caller.  If @break_nonopt is FALSE then the first non-option
 * argument concludes option processing and all subsequent options are
 * considered to be ordinary arguments; this is most useful when the
 * first argument should be a command.
 *
 * Both the array itself, and the array items are allocated with nih_alloc();
 * the items are children of the array, so it is only necessary to call
 * nih_free() on the array.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned array.  When all parents
 * of the returned array are freed, the returned array will also be
 * freed.
 *
 * Errors are handled by printing a message to standard error.
 *
 * Returns: non-option arguments array or NULL on error.
 **/
char **
nih_option_parser (const void *parent,
		   int         argc,
		   char       *argv[],
		   NihOption  *options,
		   int         break_nonopt)
{
	NihOptionCtx ctx;

	nih_assert (argc > 0);
	nih_assert (argv != NULL);
	nih_assert (options != NULL);

	ctx.parent = parent;
	ctx.argc = argc;
	ctx.argv = argv;

	ctx.options = nih_option_join (NULL, options, default_options);

	ctx.nargs = 0;
	ctx.args = NIH_MUST (nih_str_array_new (parent));

	ctx.nonopt = 0;
	ctx.optend = 0;

	/* Iterate the arguments looking for options */
	for (ctx.arg = 1; ctx.arg < argc; ctx.arg++) {
		char *arg;

		arg = ctx.argv[ctx.arg];
		if ((arg[0] != '-') || (arg[1] == '\0')
		    || (ctx.optend && ctx.arg > ctx.optend)) {
			/* Not an option */
			if (ctx.arg > ctx.nonopt) {
				NIH_MUST (nih_str_array_add
					  (&ctx.args, parent, &ctx.nargs,
					   ctx.argv[ctx.arg]));
				if (break_nonopt)
					ctx.optend = ctx.arg;
			}

		} else if (arg[1] != '-') {
			/* Short option */
			if (nih_option_short (&ctx) < 0)
				goto error;

		} else if (arg[2] != '\0') {
			/* Long option */
			if (nih_option_long (&ctx) < 0)
				goto error;

		} else {
			/* End of options */
			ctx.optend = ctx.arg;
		}
	}

	nih_free (ctx.options);
	return ctx.args;
error:
	nih_free (ctx.options);
	nih_free (ctx.args);
	return NULL;
}


/**
 * nih_option_get_short:
 * @ctx: parsing context,
 * @option: option to find.
 *
 * Find the option structure with the given short @option.  If an option
 * exists with the short option '-' this is used instead if no specific
 * option is found.
 *
 * Returns; pointer to option, or NULL if not found.
 **/
static NihOption *
nih_option_get_short (NihOptionCtx *ctx,
		      int           option)
{
	NihOption *opt, *catch = NULL;

	for (opt = ctx->options; (opt->option || opt->long_option); opt++) {
		if (opt->option == '-')
			catch = opt;

		if (opt->option == option)
			return opt;
	}

	return catch;
}

/**
 * nih_option_short:
 * @ctx: parsing context.
 *
 * Process the current argument as a list of short options, handling
 * each one individually.
 *
 * If the first option in the list expects a value, then the rest of
 * the argument is taken to be the option argument rather than further
 * options.
 *
 * Returns: zero on success, negative value on invalid option.
 **/
static int
nih_option_short (NihOptionCtx *ctx)
{
	char *ptr;

	nih_assert (ctx != NULL);
	nih_assert (program_name != NULL);

	for (ptr = ctx->argv[ctx->arg] + 1; *ptr != '\0'; ptr++) {
		NihOption *opt;

		opt = nih_option_get_short (ctx, *ptr);
		if (! opt) {
			fprintf (stderr, _("%s: invalid option: -%c\n"),
				 program_name, *ptr);
			nih_main_suggest_help ();
			return -1;
		}

		/* If the option takes an argument, this is the first
		 * option in the list and there are further characters;
		 * treat the rest as the argument
		 */
		if (opt->arg_name && (ptr[-1] == '-') && (ptr[1] != '\0')) {
			if (nih_option_handle_arg (ctx, opt, ptr + 1) < 0)
				return -1;

			break;
		}

		/* Otherwise it's an ordinary option */
		if (nih_option_handle (ctx, opt) < 0)
			return -1;
	}

	return 0;
}


/**
 * nih_option_get_long:
 * @ctx: parsing context,
 * @option: option to find,
 * @len: length of option.
 *
 * Find the option structure with the given long @option, of which only
 * the first @len characters will be read.  If an option named "--" exists
 * then it is used if no other option could be found.
 *
 * Returns; pointer to option, or NULL if not found.
 **/
static NihOption *
nih_option_get_long (NihOptionCtx *ctx,
		     const char   *option,
		     size_t        len)
{
	NihOption *opt, *catch = NULL;

	for (opt = ctx->options; (opt->option || opt->long_option); opt++) {
		if (! opt->long_option)
			continue;

		if (! strcmp (opt->long_option, "--"))
			catch = opt;

		if (strlen (opt->long_option) > len)
			continue;

		if (! strncmp (option, opt->long_option, len))
			return opt;
	}

	return catch;
}

/**
 * nih_option_long:
 * @ctx: parsing context.
 *
 * Process the current argument as a long option to be handled.
 *
 * If the option expects a value then it may be separated from the option
 * name by an '=' sign.
 *
 * Returns: zero on success, negative value on invalid option.
 **/
static int
nih_option_long (NihOptionCtx *ctx)
{
	NihOption *opt;
	char      *arg, *ptr;
	size_t     len;

	nih_assert (ctx != NULL);
	nih_assert (program_name != NULL);

	/* Check for an equals sign that separates the option name from
	 * an argument.
	 */
	arg = ctx->argv[ctx->arg] + 2;
	ptr = strchr (arg, '=');
	len = (ptr ? (size_t)(ptr - arg) : strlen (arg));

	/* Find the option */
	opt = nih_option_get_long (ctx, arg, len);
	if (! opt) {
		fprintf (stderr, _("%s: invalid option: --%s\n"),
			 program_name, arg);
		nih_main_suggest_help ();
		return -1;
	}

	/* Handle the case where there's an argument; either we need
	 * to process it, or error
	 */
	if (ptr != NULL) {
		if (opt->arg_name) {
			if (nih_option_handle_arg (ctx, opt, ptr + 1) < 0)
				return -1;

			return 0;
		} else {
			fprintf (stderr, _("%s: unexpected argument: --%s\n"),
				 program_name, arg);
			nih_main_suggest_help ();
			return -1;
		}
	}

	/* Otherwise it's an ordinary option */
	if (nih_option_handle (ctx, opt) < 0)
		return -1;

	return 0;
}


/**
 * nih_option_handle:
 * @ctx: parsing context,
 * @opt: option to handle.
 *
 * Handle an option which either does not take an argument, or should
 * take the next non-option argument from the command-line.  For options
 * with arguments, this calls nih_option_handle_arg(); for those without,
 * this calls the setter function or treats the value member as a pointer
 * to an int to store TRUE in.
 *
 * Returns: zero on success, negative value on invalid option.
 **/
static int
nih_option_handle (NihOptionCtx *ctx,
		   NihOption    *opt)
{
	nih_assert (ctx != NULL);
	nih_assert (opt != NULL);

	/* Handle the special cased options first */
	if (opt->long_option && (! strcmp (opt->long_option, "help"))) {
		/* --help */
		nih_option_help (ctx->options);
		nih_free (ctx->options);
		nih_free (ctx->args);
		exit (0);
	} else if (opt->long_option
		   && (! strcmp (opt->long_option, "version"))) {
		/* --version */
		nih_main_version ();
		nih_free (ctx->options);
		nih_free (ctx->args);
		exit (0);
	}

	if (opt->arg_name) {
		const char *arg;

		arg = nih_option_next_nonopt (ctx);
		if (! arg) {
			fprintf (stderr, _("%s: missing argument: %s\n"),
				 program_name, ctx->argv[ctx->arg]);
			nih_main_suggest_help ();
			return -1;
		}

		return nih_option_handle_arg (ctx, opt, arg);
	} else if (opt->setter) {
		return opt->setter (opt, NULL);
	} else if (opt->value) {
		int *value = (int *)opt->value;

		*value = TRUE;
	}

	return 0;
}

/**
 * nih_option_handle_arg:
 * @ctx: parsing context,
 * @opt: option to handle,
 * @arg: argument.
 *
 * Handle an option which has the argument specified, either calling
 * the setter function or treating the value member (if present) as the
 * address of a char * to store the duplicated argument value in.
 *
 * Returns: zero on success, negative value on invalid option.
 **/
static int
nih_option_handle_arg (NihOptionCtx *ctx,
		       NihOption    *opt,
		       const char   *arg)
{
	nih_assert (ctx != NULL);
	nih_assert (opt != NULL);
	nih_assert (opt->arg_name != NULL);
	nih_assert (arg != NULL);

	if (opt->setter) {
		return opt->setter (opt, arg);
	} else if (opt->value) {
		char **value = (char **)opt->value;

		if (*value)
			nih_free (*value);

		*value = NIH_MUST (nih_strdup (ctx->parent, arg));
	}

	return 0;
}


/**
 * nih_option_next_nonopt:
 * @ctx: parsing context.
 *
 * Iterates the command-line arguments looking for the next argument
 * that is not an option.  Updates the nonopt member of @ctx to point
 * at the option used.
 *
 * Returns: next non-option argument or NULL if none remain.
 **/
static const char *
nih_option_next_nonopt (NihOptionCtx *ctx)
{
	nih_assert (ctx != NULL);

	if (ctx->nonopt < ctx->arg)
		ctx->nonopt = ctx->arg;

	while (++ctx->nonopt < ctx->argc) {
		char *arg;

		arg = ctx->argv[ctx->nonopt];
		if ((arg[0] != '-')
		    || (ctx->optend && ctx->nonopt > ctx->optend)) {
			return arg;

		} else if ((arg[1] == '-') && (arg[2] == '\0')) {
			/* End of options */
			ctx->optend = ctx->nonopt;
		}
	}

	return NULL;
}


/**
 * nih_option_join:
 * @parent: parent object for new array,
 * @a: first option array,
 * @b: second option array.
 *
 * Joins the two option arrays together to produce a combined array containing
 * the options from @a followed by the options from @b.
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
 * Returns: combined option array.
 **/
NihOption *
nih_option_join (const void      *parent,
		 const NihOption *a,
		 const NihOption *b)
{
	const NihOption *opt;
	NihOption       *opts;
	size_t           alen = 0, blen = 0;

	nih_assert (a != NULL);
	nih_assert (b != NULL);

	/* Count options in first list */
	for (opt = a; (opt->option || opt->long_option); opt++)
		alen++;

	/* Count options in second list */
	for (opt = b; (opt->option || opt->long_option); opt++)
		blen++;

	/* Allocate combined list */
	opts = NIH_MUST (nih_alloc (parent,
				    sizeof (NihOption) * (alen + blen + 1)));

	/* Copy options, making sure to copy the last option from b */
	memcpy (opts, a, sizeof (NihOption) * alen);
	memcpy (opts + alen, b, sizeof (NihOption) * (blen + 1));

	return opts;
}


/**
 * nih_option_count:
 * @option: NihOption invoked,
 * @arg: argument to parse.
 *
 * This option setter may be used to create arguments that count the number
 * of times they are placed on the command line.
 *
 * The value member of @option must be a pointer to an integer variable,
 * the arg_name member must be NULL.
 *
 * Returns: always returns zero.
 **/
int
nih_option_count (NihOption  *option,
		  const char *arg)
{
	int *value;

	nih_assert (option != NULL);
	nih_assert (option->value != NULL);
	nih_assert (arg == NULL);

	value = (int *)option->value;
	(*value)++;

	return 0;
}

/**
 * nih_option_int:
 * @option: NihOption invoked,
 * @arg: argument to parse.
 *
 * This option setter may be used to parse an integer from the command line
 * and store it in the value member of @option, which must be a pointer to
 * an integer variable.
 *
 * The arg_name member of @option must not be NULL.
 *
 * Returns: zero on success, non-zero on error.
 **/
int
nih_option_int (NihOption  *option,
		const char *arg)
{
	char *endptr;
	int  *value;

	nih_assert (option != NULL);
	nih_assert (option->value != NULL);
	nih_assert (arg != NULL);

	value = (int *)option->value;
	*value = strtol (arg, &endptr, 10);

	if (*endptr) {
		fprintf (stderr, _("%s: illegal argument: %s\n"),
			 program_name, arg);
		nih_main_suggest_help ();
		return -1;
	}

	return 0;
}


/**
 * nih_option_quiet:
 * @option: NihOption invoked,
 * @arg: argument to parse.
 *
 * This option setter is used by the built-in -q/--quiet option to set the
 * default logging level to ERROR.
 *
 * Returns: always returns zero.
 **/
int
nih_option_quiet (NihOption  *option,
		  const char *arg)
{
	nih_assert (option != NULL);
	nih_assert (arg == NULL);

	nih_log_set_priority (NIH_LOG_ERROR);

	return 0;
}

/**
 * nih_option_verbose:
 * @option: NihOption invoked,
 * @arg: argument to parse.
 *
 * This option setter is used by the built-in -v/--verbose option to set the
 * default logging level to INFO.
 *
 * Returns: always returns zero.
 **/
int
nih_option_verbose (NihOption  *option,
		    const char *arg)
{
	nih_assert (option != NULL);
	nih_assert (arg == NULL);

	nih_log_set_priority (NIH_LOG_INFO);

	return 0;
}

/**
 * nih_option_debug:
 * @option: NihOption invoked,
 * @arg: argument to parse.
 *
 * This option setter is used by the built-in --debug option to set the
 * default logging level to DEBUG.
 *
 * Returns: always returns zero.
 **/
int
nih_option_debug (NihOption  *option,
		  const char *arg)
{
	nih_assert (option != NULL);
	nih_assert (arg == NULL);

	nih_log_set_priority (NIH_LOG_DEBUG);

	return 0;
}


/**
 * nih_option_set_usage_stem:
 * @usage: usage stem.
 *
 * Set the usage stem prepended to the program usage line in the help output,
 * this should be a static translated string.
 *
 * The string should not be terminated with a newline.
 **/
void
nih_option_set_usage_stem (const char *usage)
{
	usage_stem = usage;
}

/**
 * nih_option_set_usage:
 * @usage: usage string.
 *
 * Set the usage string appended to the program usage line in the help output,
 * this should be a static translated string.
 *
 * The string should not be terminated with a newline.
 **/
void
nih_option_set_usage (const char *usage)
{
	usage_string = usage;
}

/**
 * nih_option_set_synopsis:
 * @synopsis: synopsis string.
 *
 * Set the synopsis string, shown after the program usage line in the help
 * output.  This should be a static translated string.  It will be
 * automatically wrapped to the screen width.
 *
 * The string should not be terminated with a newline.
 **/
void
nih_option_set_synopsis (const char *synopsis)
{
	synopsis_string = synopsis;
}

/**
 * nih_option_set_help:
 * @help: help string.
 *
 * Set the help string, this is displayed after the options in the help
 * output.  This should be a static translated string.  It will be
 * automatically wrapped to the screen width.
 *
 * The string should not be terminated with a newline.
 **/
void
nih_option_set_help (const char *help)
{
	help_string = help;
}

/**
 * nih_option_set_footer:
 * @footer: footer string.
 *
 * Set the footer string, this is displayed after the options and help
 * text in the output.  This should be a static translated string.
 *
 * The string should not be terminated with a newline.
 **/
void
nih_option_set_footer (const char *footer)
{
	footer_string = footer;
}

/**
 * nih_option_help:
 * @options: program options list.
 *
 * Output a description of the program's options to standard output
 * grouped by the group member of the option.
 **/
static void
nih_option_help (NihOption *options)
{
	NihOption                 *opt;
	nih_local NihOptionGroup **groups = NULL;
	size_t                     group, ngroups;
	int                        other = FALSE;

	nih_assert (program_name != NULL);

	/* Count the number of option groups */
	ngroups = 0;
	for (opt = options; (opt->option || opt->long_option); opt++) {
		if (! opt->group) {
			other = TRUE;
			continue;
		}

		for (group = 0; group < ngroups; group++) {
			if (groups[group] == opt->group)
				break;
		}

		if (group < ngroups)
			continue;

		groups = NIH_MUST (nih_realloc (groups, NULL,
						    (sizeof (NihOptionGroup *)
						     * (ngroups + 1))));
		groups[ngroups++] = opt->group;
	}

	printf ("%s: %s", _("Usage"), program_name);
	if (usage_stem) {
		printf (" %s", usage_stem);
	} else {
		printf (" %s", _("[OPTION]..."));
	}
	if (usage_string)
		printf (" %s", usage_string);
	printf ("\n");

	/* Wrap the synopsis to the screen width */
	if (synopsis_string) {
		nih_local char *str;

		str = NIH_MUST (nih_str_screen_wrap (NULL, synopsis_string,
						     0, 0));
		printf ("%s\n", str);
	}
	printf ("\n");

	/* Iterate the option groups we found in order, and display
	 * only their options
	 */
	for (group = 0; group < ngroups; group++)
		nih_option_group_help (groups[group], options, groups);

	/* Display the other group */
	if (other)
		nih_option_group_help (NULL, options, groups);

	/* Wrap the help to the screen width */
	if (help_string) {
		nih_local char *str;

		str = NIH_MUST (nih_str_screen_wrap (NULL, help_string, 0, 0));
		printf ("%s\n", str);

		if (package_bugreport || footer_string)
			printf ("\n");
	}

	/* Append the footer */
	if (footer_string) {
		printf ("%s\n", footer_string);

		if (package_bugreport)
			printf ("\n");
	}

	/* Append the bug report address */
	if (package_bugreport) {
		if (strchr (package_bugreport, '@')) {
			printf (_("Report bugs to <%s>\n"), package_bugreport);
		} else {
			printf (_("Report bugs at <%s>\n"), package_bugreport);
		}
	}
}

/**
 * nih_option_group_help:
 * @group: group to display,
 * @options: program options list,
 * @groups: all groups.
 *
 * Output a list of the program's options in the given option group to
 * standard output.
 **/
static void
nih_option_group_help (NihOptionGroup  *group,
		       NihOption       *options,
		       NihOptionGroup **groups)
{
	NihOption *opt;
	size_t     width;

	nih_assert (options != NULL);

	if (group) {
		printf (_("%s options:\n"), _(group->title));
	} else if (groups) {
		printf (_("Other options:\n"));
	} else {
		printf (_("Options:\n"));
	}

	width = nih_max (nih_str_screen_width (), 50U) - 31;

	for (opt = options; (opt->option || opt->long_option); opt++) {
		nih_local char *str = NULL;
		char           *ptr;
		size_t          len = 0;

		if (opt->group != group)
			continue;

		if (! opt->help)
			continue;

		/* Indent by two spaces */
		printf ("  ");
		len += 2;

		/* Display the short option */
		if (opt->option) {
			printf ("-%c", opt->option);
			len += 2;

			/* Seperate short and long option, or
			 * give the argument name
			 */
			if (opt->long_option) {
				printf (", ");
				len += 2;
			} else if (opt->arg_name) {
				printf (" %s", opt->arg_name);
				len += strlen (opt->arg_name) + 1;
			}
		} else {
			/* Make all long options the same indent
			 * whether or not there's a short one
			 */
			printf ("    ");
			len += 4;
		}

		/* Display the long option */
		if (opt->long_option) {
			printf ("--%s", opt->long_option);
			len += strlen (opt->long_option) + 2;

			/* With the argument name */
			if (opt->arg_name) {
				printf ("=%s", opt->arg_name);
				len += strlen (opt->arg_name) + 1;
			}
		}

		/* Format the help string to fit in the latter
		 * half of the screen
		 */
		str = NIH_MUST (nih_str_wrap (NULL, opt->help, width, 0, 2));

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
