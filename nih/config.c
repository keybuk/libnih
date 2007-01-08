/* libnih
 *
 * config.c - configuration file parsing
 *
 * Copyright © 2007 Scott James Remnant <scott@netsplit.com>.
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <sys/types.h>

#include <limits.h>
#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/file.h>
#include <nih/config.h>
#include <nih/logging.h>
#include <nih/error.h>
#include <nih/errors.h>


/**
 * WS:
 *
 * Definition of what characters we consider whitespace.
 **/
#define WS " \t\r"

/**
 * CNL:
 *
 * Definition of what characters nominally end a line; a comment start
 * character or a newline.
 **/
#define CNL "#\n"

/**
 * CNLWS:
 *
 * Defintion of what characters nominally separate tokens.
 **/
#define CNLWS " \t\r#\n"



/* Prototypes for static functions */
static ssize_t          nih_config_block_end  (const char *file, size_t len,
					       size_t *lineno, size_t *pos,
					       const char *type);
static NihConfigStanza *nih_config_get_stanza (const char *name,
					       NihConfigStanza *stanzas);


/**
 * nih_config_next_token:
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
 * @lineno: line number,
 * @dest: destination to copy to,
 * @delim: characters to stop on,
 * @dequote: remove quotes and escapes.
 *
 * Extracts a single token from @file which is stopped when any character
 * in @delim is encountered outside of a quoted string and not escaped
 * using a backslash.
 *
 * @file may be a memory mapped file, in which case @pos should be given
 * as the offset within and @len should be the length of the file as a
 * whole.  Usually when @dest is given, @file is instead the pointer to
 * the start of the token and @len is the difference between the start
 * and end of the token (NOT the return value from this function).
 *
 * If @pos is given then it will be used as the offset within @file to
 * begin (otherwise the start is assumed), and will be updated to point
 * to @delim or past the end of the file.
 *
 * If @lineno is given it will be incremented each time a new line is
 * discovered in the file.
 *
 * To copy the token into another string, collapsing any newlines and
 * surrounding whitespace to a single space, pass @dest which should be
 * pre-allocated to the right size (obtained by calling this function
 * with NULL).
 *
 * If you also want quotes to be removed and escaped characters to be
 * replaced with the character itself, set @dequote to TRUE.
 *
 * Returns: the length of the token as it was/would be copied into @dest,
 * or negative value on raised error.
 **/
ssize_t
nih_config_next_token (const char *file,
		       size_t      len,
		       size_t     *pos,
		       size_t     *lineno,
		       char       *dest,
		       const char *delim,
		       int         dequote)
{
	size_t  p, ws = 0, nlws = 0, qc = 0, i = 0;
	ssize_t ret;
	int     slash = FALSE, quote = 0, nl = FALSE;

	nih_assert (file != NULL);
	nih_assert (delim != NULL);

	/* We keep track of the following:
	 *   slash  whether a \ is in effect
	 *   quote  whether " or ' is in effect (set to which)
	 *   ws     number of consecutive whitespace chars so far
	 *   nlws   number of whitespace/newline chars
	 *   nl     TRUE if we need to copy ws into nlws at first non-WS
	 *   qc     number of quote characters that need removing.
	 */

	for (p = (pos ? *pos : 0); p < len; p++) {
		int extra = 0, isq = FALSE;

		if (slash) {
			slash = FALSE;

			/* Escaped newline */
			if (file[p] == '\n') {
				nlws++;
				nl = TRUE;
				if (lineno)
					(*lineno)++;
				continue;
			} else {
				extra++;
				if (dequote)
					qc++;
			}
		} else if (file[p] == '\\') {
			slash = TRUE;
			continue;
		} else if (quote) {
			if (file[p] == quote) {
				quote = 0;
				isq = TRUE;
			} else if (file[p] == '\n') {
				nl = TRUE;
				if (lineno)
					(*lineno)++;
				continue;
			} else if (strchr (WS, file[p])) {
				ws++;
				continue;
			}
		} else if ((file[p] == '\"') || (file[p] == '\'')) {
			quote = file[p];
			isq = TRUE;
		} else if (strchr (delim, file[p])) {
			break;
		} else if (strchr (WS, file[p])) {
			ws++;
			continue;
		}

		if (nl) {
			/* Newline is recorded as a single space;
			 * any surrounding whitespace is lost.
			 */
			nlws += ws;
			if (dest)
				dest[i++] = ' ';
		} else if (ws && dest) {
			/* Whitespace that we've encountered to date is
			 * copied as it is.
			 */
			memcpy (dest + i, file + p - ws - extra, ws);
			i += ws;
		}

		/* Extra characters (the slash) needs to be copied
		 * unless we're dequoting the string
		 */
		if (extra && dest && (! dequote)) {
			memcpy (dest + i, file + p - extra, extra);
			i += extra;
		}

		if (dest && (! (isq && dequote)))
			dest[i++] = file[p];

		if (isq && dequote)
			qc++;

		ws = 0;
		nl = FALSE;
		extra = 0;
	}

	/* Add the NULL byte */
	if (dest)
		dest[i++] = '\0';


	/* A trailing slash on the end of the file makes no sense. */
	if (slash) {
		nih_error_raise (NIH_CONFIG_TRAILING_SLASH,
				 _(NIH_CONFIG_TRAILING_SLASH_STR));
		ret = -1;
		goto finish;
	}

	/* Leaving quotes open is also generally bad. */
	if (quote) {
		nih_error_raise (NIH_CONFIG_UNTERMINATED_QUOTE,
				 _(NIH_CONFIG_UNTERMINATED_QUOTE_STR));
		ret = -1;
		goto finish;
	}


	/* The return value is the length of the token with any newlines and
	 * surrounding whitespace converted to a single character and any
	 * trailing whitespace removed.
	 *
	 * The actual end of the text read is returned in *pos.
	 */
	ret = p - (pos ? *pos : 0) - ws - nlws - qc;

finish:
	if (pos)
		*pos = p;

	return ret;
}

/**
 * nih_config_next_arg:
 * @parent: parent of returned argument,
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
 * @lineno: line number.
 *
 * Extracts a single argument from @file, a dequoted token that is stopped
 * on any comment, space or newline character that is not quoted or escaped
 * with a backslash.  Any whitespace after the argument is also consumed,
 * but not returned, including that with escaped newlines within it.
 *
 * @file may be a memory mapped file, in which case @pos should be given
 * as the offset within and @len should be the length of the file as a
 * whole.
 *
 * If @pos is given then it will be used as the offset within @file to
 * begin (otherwise the start is assumed), and will be updated to point
 * to @delim or past the end of the file.
 *
 * If @lineno is given it will be incremented each time a new line is
 * discovered in the file.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned block will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: the argument found or NULL on raised error.
 **/
char *
nih_config_next_arg (const void *parent,
		     const char *file,
		     size_t      len,
		     size_t     *pos,
		     size_t     *lineno)
{
	size_t   p, arg_start, arg_end;
	ssize_t  arg_len;
	char    *arg = NULL;

	nih_assert (file != NULL);

	p = (pos ? *pos : 0);
	arg_start = p;
	arg_len = nih_config_next_token (file, len, &p, lineno,
					 NULL, CNLWS, TRUE);
	arg_end = p;

	if (arg_len < 0) {
		goto finish;
	} else if (! arg_len) {
		nih_error_raise (NIH_CONFIG_EXPECTED_TOKEN,
				 _(NIH_CONFIG_EXPECTED_TOKEN_STR));
		goto finish;
	}

	/* Skip any amount of whitespace between them, we also need to
	 * detect an escaped newline here.
	 */
	while (p < len) {
		if (file[p] == '\\') {
			/* Escape character, only continue scanning if
			 * the next character is newline
			 */
			if ((len - p > 1) && (file[p + 1] == '\n')) {
				p++;
			} else {
				break;
			}
		} else if (! strchr (WS, file[p])) {
			break;
		}

		if (file[p] == '\n')
			if (lineno)
				(*lineno)++;

		/* Whitespace characer */
		p++;
	}

	/* Copy in the new token */
	NIH_MUST (arg = nih_alloc (parent, arg_len + 1));
	nih_config_next_token (file + arg_start, arg_end - arg_start, NULL,
			       NULL, arg, CNLWS, TRUE);

finish:
	if (pos)
		*pos = p;

	return arg;
}

/**
 * nih_config_next_line:
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
 * @lineno: line number.
 *
 * Skips to the end of the current line in @file, ignoring any tokens,
 * comments, etc. along the way.  If you want to ensure that no arguments
 * are missed, use nih_config_skip_comment() instead.
 *
 * @file may be a memory mapped file, in which case @pos should be given
 * as the offset within and @len should be the length of the file as a
 * whole.
 *
 * @pos is used as the offset within @file to begin, and will be updated
 * to point to past the end of the line or file.
 *
 * If @lineno is given it will be incremented each time a new line is
 * discovered in the file.
 **/
void
nih_config_next_line (const char *file,
		      size_t      len,
		      size_t     *pos,
		      size_t     *lineno)
{
	nih_assert (file != NULL);
	nih_assert (pos != NULL);

	/* Spool forwards until the end of the line */
	while ((*pos < len) && (file[*pos] != '\n'))
		(*pos)++;

	/* Step over it */
	if (*pos < len) {
		if (lineno)
			(*lineno)++;
		(*pos)++;
	}
}

/**
 * nih_config_skip_comment:
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
 * @lineno: line number.
 *
 * Skips a comment and finds the end of the current line in @file.  If the
 * current position does not point to the end of a line, or a comment,
 * then an error is raised.
 *
 * @file may be a memory mapped file, in which case @pos should be given
 * as the offset within and @len should be the length of the file as a
 * whole.
 *
 * @pos is used as the offset within @file to begin, and will be updated
 * to point to past the end of the line or file.
 *
 * If @lineno is given it will be incremented each time a new line is
 * discovered in the file.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
nih_config_skip_comment (const char *file,
			 size_t      len,
			 size_t     *pos,
			 size_t     *lineno)
{
	nih_assert (file != NULL);
	nih_assert (pos != NULL);

	if ((*pos < len) && (! strchr (CNL, file[*pos]))) {
		nih_error_raise (NIH_CONFIG_UNEXPECTED_TOKEN,
				 _(NIH_CONFIG_UNEXPECTED_TOKEN_STR));
		return -1;
	}

	nih_config_next_line (file, len, pos, lineno);

	return 0;
}


/**
 * nih_config_parse_args:
 * @parent: parent of returned array,
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
 * @lineno: line number.
 *
 * Extracts a list of arguments from @file, each argument is separated
 * by whitespace and parsing is stopped when a newline is encountered
 * outside of a quoted string and not escaped using a backslash.
 *
 * @file may be a memory mapped file, in which case @pos should be given
 * as the offset within and @len should be the length of the file as a
 * whole.
 *
 * If @pos is given then it will be used as the offset within @file to
 * begin (otherwise the start is assumed), and will be updated to point
 * past the end of the line or the end of the file.
 *
 * If @lineno is given it will be incremented each time a new line is
 * discovered in the file.
 *
 * The arguments are returned as a NULL-terminated array, with each argument
 * dequoted before being returned.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned block will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: the list of arguments found or NULL on raised error.
 **/
char **
nih_config_parse_args (const void *parent,
		       const char *file,
		       size_t      len,
		       size_t     *pos,
		       size_t     *lineno)
{
	char   **args;
	size_t   p, nargs;

	nih_assert (file != NULL);

	/* Begin with an empty array */
	NIH_MUST (args = nih_alloc (parent, sizeof (char *)));
	args[0] = NULL;
	nargs = 0;

	/* Loop through the arguments until we hit a comment or newline */
	p = (pos ? *pos : 0);
	while ((p < len) && (! strchr (CNL, file[p]))) {
		char  **new_args, *arg;

		arg = nih_config_next_arg (args, file, len, &p, lineno);
		if (! arg) {
			nih_free (args);
			args = NULL;
			goto finish;
		}

		/* Extend the array and add the new argument. */
		NIH_MUST (new_args = nih_realloc (args, parent,
						  (sizeof (char *)
						   * (nargs + 2))));

		args = new_args;
		args[nargs++] = arg;
		args[nargs] = NULL;

	}

	if (nih_config_skip_comment (file, len, &p, lineno) < 0) {
		nih_free (args);
		args = NULL;
		goto finish;
	}

finish:
	if (pos)
		*pos = p;

	return args;
}

/**
 * nih_config_parse_command:
 * @parent: parent of returned string,
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
 * @lineno: line number,
 *
 * Extracts a command and its arguments from @file, stopping when a
 * newline is encountered outside of a quoted string and not escaped
 * using a blackslash.
 *
 * @file may be a memory mapped file, in which case @pos should be given
 * as the offset within and @len should be the length of the file as a
 * whole.
 *
 * If @pos is given then it will be used as the offset within @file to
 * begin (otherwise the start is assumed), and will be updated to point
 * past the end of the line or the end of the file.
 *
 * If @lineno is given it will be incremented each time a new line is
 * discovered in the file.
 *
 * The command is returned as a string allocated with nih_alloc().
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned block will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: the command found or NULL on raised error.
 **/
char *
nih_config_parse_command (const void *parent,
			  const char *file,
			  size_t      len,
			  size_t     *pos,
			  size_t     *lineno)
{
	char    *cmd = NULL;
	size_t   p, cmd_start, cmd_end;
	ssize_t  cmd_len;

	nih_assert (file != NULL);

	/* Find the length of string up to the first unescaped comment
	 * or newline.
	 */
	p = (pos ? *pos : 0);
	cmd_start = p;
	cmd_len = nih_config_next_token (file, len, &p, lineno,
					 NULL, CNL, FALSE);
	cmd_end = p;

	if (cmd_len < 0)
		goto finish;

	if (nih_config_skip_comment (file, len, &p, lineno) < 0)
		goto finish;

	/* Now copy the string into the destination. */
	NIH_MUST (cmd = nih_alloc (parent, cmd_len + 1));
	nih_config_next_token (file + cmd_start, cmd_end - cmd_start, NULL,
			       NULL, cmd, CNL, FALSE);

finish:
	if (pos)
		*pos = p;

	return cmd;
}


/**
 * nih_config_parse_block:
 * @parent: parent of returned string,
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
 * @lineno: line number,
 * @type: block identifier.
 *
 * Extracts a block of text from @line, stopping when the pharse "end @type"
 * is encountered without any quotes or blackslash escaping within it.
 *
 * @file may be a memory mapped file, in which case @pos should be given
 * as the offset within and @len should be the length of the file as a
 * whole.
 *
 * If @pos is given then it will be used as the offset within @file to
 * begin (otherwise the start is assumed), and will be updated to point
 * past the end of the block or the end of the file.
 *
 * Either @file or @pos should point to the start of the block, after the
 * opening stanza, rather than the start of the stanza that opens it.
 *
 * If @lineno is given it will be incremented each time a new line is
 * discovered in the file.
 *
 * The block is returned as a string allocated with nih_alloc().
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned block will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: the text contained within the block or NULL on raised error.
 **/
char *
nih_config_parse_block (const void *parent,
			const char *file,
			size_t      len,
			size_t     *pos,
			size_t     *lineno,
			const char *type)
{
	char    *block = NULL;
	size_t   p, pp, sh_start, sh_len;
	ssize_t  sh_end, ws;
	int      lines;

	nih_assert (file != NULL);
	nih_assert (type != NULL);

	/* We need to find the end of the block which is a line that looks
	 * like:
	 *
	 * 	WS? end WS @type CNLWS?
	 *
	 * Just to make things more difficult for ourselves, we work out the
	 * common whitespace on the start of the block lines and remember
	 * not to copy those out later
	 */
	p = (pos ? *pos : 0);
	sh_start = p;
	sh_end = -1;
	ws = -1;
	lines = 0;

	while ((sh_end = nih_config_block_end (file, len, &p,
					       lineno, type)) < 0) {
		size_t line_start;

		lines++;
		line_start = p;
		if (ws < 0) {
			/* Count initial whitespace */
			while ((p < len) && strchr (WS, file[p]))
				p++;

			ws = p - line_start;
		} else {
			/* Compare how much whitespace matches the
			 * first line; and decrease the count if it's
			 * not as much.
			 */
			while ((p < len) && (p - line_start < ws)
			       && (file[sh_start + p - line_start] == file[p]))
				p++;

			if (p - line_start < ws)
				ws = p - line_start;
		}

		nih_config_next_line (file, len, &p, lineno);

		if (p >= len) {
			nih_error_raise (NIH_CONFIG_UNTERMINATED_BLOCK,
					 _(NIH_CONFIG_UNTERMINATED_BLOCK_STR));
			goto finish;
		}
	}

	/* Copy the fragment into a string, removing common whitespace from
	 * the start.  We can be less strict here because we already know
	 * the contents, etc.
	 */
	sh_len = sh_end - sh_start - (ws * lines);
	NIH_MUST (block = nih_alloc (parent, sh_len + 1));
	block[0] = '\0';

	pp = sh_start;
	while (pp < sh_end) {
		size_t line_start;

		pp += ws;
		line_start = pp;

		while (file[pp++] != '\n')
			;

		strncat (block, file + line_start, pp - line_start);
	}

finish:
	if (pos)
		*pos = p;

	return block;
}

/**
 * nih_config_block_end:
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
 * @lineno: line number,
 * @type: block identifier.
 *
 * Determines whether the current line contains an end of block marker.
 *
 * @file may be a memory mapped file, in which case @pos should be given
 * as the offset within and @len should be the length of the file.  @pos
 * will be updated to point past the end of the block or the end of the
 * file.
 *
 * @lineno will be incremented each time a new line is discovered.
 *
 * Returns: index of block end (always the value of @pos at the time this
 * function was called) or -1 if it is not on this line.
 **/
static ssize_t
nih_config_block_end (const char *file,
		      size_t      len,
		      size_t     *pos,
		      size_t     *lineno,
		      const char *type)
{
	size_t  p;
	ssize_t end;

	nih_assert (file != NULL);
	nih_assert (pos != NULL);
	nih_assert (type != NULL);

	p = *pos;

	/* Skip initial whitespace */
	while ((p < len) && strchr (WS, file[p]))
		p++;

	/* Check the first word (check we have at least 4 chars because of
	 * the need for whitespace immediately after)
	 */
	if ((len - p < 4) || strncmp (file + p, "end", 3))
		return -1;

	/* Must be whitespace after */
	if (! strchr (WS, file[p + 3]))
		return -1;

	/* Find the second word */
	p += 3;
	while ((p < len) && strchr (WS, file[p]))
		p++;

	/* Check the second word */
	if ((len - p < strlen (type))
	    || strncmp (file + p, type, strlen (type)))
		return -1;

	/* May be followed by whitespace */
	p += strlen (type);
	while ((p < len) && strchr (WS, file[p]))
		p++;

	/* May be a comment, in which case eat up to the newline
	 */
	if ((p < len) && (file[p] == '#')) {
		while ((p < len) && (file[p] != '\n'))
			p++;
	}

	/* Should be end of string, or a newline */
	if ((p < len) && (file[p] != '\n'))
		return -1;

	/* Point past the new line */
	if (p < len) {
		if (lineno)
			(*lineno)++;
		p++;
	}

	/* Return the beginning of the line (which is the end of the script)
	 * but update pos to point past this line.
	 */
	end = *pos;
	*pos = p;

	return end;
}


/**
 * nih_config_get_stanza:
 * @name: name of stanza,
 * @stanzas: table of stanza handlers.
 *
 * Locates the handler for the @name stanza in the @stanzas table.  The
 * last entry in the table should have NULL for both the name and handler
 * function pointers.
 *
 * Returns: stanza found or NULL if no handler for @name.
 **/
static NihConfigStanza *
nih_config_get_stanza (const char      *name,
		       NihConfigStanza *stanzas)
{
	NihConfigStanza *stanza;

	for (stanza = stanzas; (stanza->name && stanza->handler); stanza++) {
		if (! strcmp (stanza->name, name))
			return stanza;
	}

	return NULL;
}

/**
 * nih_config_parse_stanza:
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
 * @lineno: line number,
 * @stanzas: table of stanza handlers,
 * @data: pointer to pass to stanza handler.
 *
 * Extracts a configuration stanza from @file and calls the handler
 * function for that stanza found in the @stanzas table to handle the
 * rest of the line from thereon in.
 *
 * @file may be a memory mapped file, in which case @pos should be given
 * as the offset within and @len should be the length of the file as a
 * whole.
 *
 * If @pos is given then it will be used as the offset within @file to
 * begin (otherwise the start is assumed), and will be updated to point
 * to @delim or past the end of the file.
 *
 * If @lineno is given it will be incremented each time a new line is
 * discovered in the file.
 *
 * A warning is output if no stanza is found, and the position is moved
 * past the end of the line.  This may result in inconsistent parsing.
 *
 * Returns: zero on success or negative value on raised error.
 **/
int
nih_config_parse_stanza (const char      *file,
			 size_t           len,
			 size_t          *pos,
			 size_t          *lineno,
			 NihConfigStanza *stanzas,
			 void            *data)
{
	NihConfigStanza *stanza;
	char            *name;
	size_t           p;
	int              ret = -1;

	nih_assert (file != NULL);
	nih_assert (stanzas != NULL);

	p = (pos ? *pos : 0);

	/* Get the next dequoted argument from the file */
	name = nih_config_next_arg (NULL, file, len, &p, lineno);
	if (! name)
		goto finish;

	/* Lookup the stanza for it */
	stanza = nih_config_get_stanza (name, stanzas);
	if (! stanza) {
		nih_error_raise (NIH_CONFIG_UNKNOWN_STANZA,
				 _(NIH_CONFIG_UNKNOWN_STANZA_STR));
		goto finish;
	}

	ret = stanza->handler (data, stanza, file, len, &p, lineno);

finish:
	if (name)
		nih_free (name);

	if (pos)
		*pos = p;

	return ret;
}


/**
 * nih_config_parse_file:
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
 * @lineno: line number,
 * @stanzas: table of stanza handlers,
 * @data: pointer to pass to stanza handler.
 *
 * Parses configuration file lines from @file, skipping initial whitespace,
 * blank lines and comments while calling nih_config_parse_stanza() for
 * anything else.
 *
 * @file may be a memory mapped file, in which case @pos should be given
 * as the offset within and @len should be the length of the file as a
 * whole.
 *
 * If @pos is given then it will be used as the offset within @file to
 * begin (otherwise the start is assumed), and will be updated to point
 * to @delim or past the end of the file.
 *
 * If @lineno is given it will be incremented each time a new line is
 * discovered in the file.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
nih_config_parse_file (const char      *file,
		       size_t           len,
		       size_t          *pos,
		       size_t          *lineno,
		       NihConfigStanza *stanzas,
		       void            *data)
{
	int    ret = -1;
	size_t p;

	nih_assert (file != NULL);
	nih_assert (stanzas != NULL);

	p = (pos ? *pos : 0);

	while (p < len) {
		/* Skip initial whitespace */
		while ((p < len) && strchr (WS, file[p]))
			p++;

		/* Skip lines with only comments in them */
		if ((p < len) && strchr (CNL, file[p])) {
			if (nih_config_skip_comment (file, len,
						     &p, lineno) < 0)
				goto finish;

			continue;
		}

		/* Must have a stanza, parse it */
		if (p < len) {
			if (nih_config_parse_stanza (file, len, &p, lineno,
						     stanzas, data) < 0)
				goto finish;
		}
	}

	ret = 0;

finish:
	if (pos)
		*pos = p;

	return ret;
}

/**
 * nih_config_parse:
 * @filename: name of file to parse,
 * @pos: offset within @file,
 * @lineno: line number,
 * @stanzas: table of stanza handlers,
 * @data: pointer to pass to stanza handler.
 *
 * Maps @filename into memory and them parses configuration lines from it
 * using nih_config_parse_file().
 *
 * If @pos is given then it will be used as the offset within @file to
 * begin (otherwise the start is assumed), and will be updated to point
 * to @delim or past the end of the file.
 *
 * If @lineno is given it will be incremented each time a new line is
 * discovered in the file.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
nih_config_parse (const char      *filename,
		  size_t          *pos,
		  size_t          *lineno,
		  NihConfigStanza *stanzas,
		  void            *data)
{
	const char *file;
	size_t      len;
	int         ret;

	nih_assert (filename != NULL);

	file = nih_file_map (filename, O_RDONLY | O_NOCTTY, &len);
	if (! file)
		return -1;

	if (len > SSIZE_MAX) {
		nih_error_raise (NIH_CONFIG_TOO_LONG,
				 _(NIH_CONFIG_TOO_LONG_STR));
		ret = -1;
		goto finish;
	}

	if (lineno)
		*lineno = 1;

	ret = nih_config_parse_file (file, len, pos, lineno, stanzas, data);

finish:
	if (nih_file_unmap ((void *)file, len) < 0)
		return -1;

	return ret;
}
