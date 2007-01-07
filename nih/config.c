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

#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/file.h>
#include <nih/config.h>
#include <nih/logging.h>
#include <nih/error.h>


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
static ssize_t          nih_config_block_end  (ssize_t *lineno,
					       const char *file, ssize_t len,
					       ssize_t *pos, const char *type);
static NihConfigStanza *nih_config_get_stanza (const char *name,
					       NihConfigStanza *stanzas);


/**
 * nih_config_next_token:
 * @filename: name of file being parsed,
 * @lineno: line number,
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
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
 * If you want warnings to be output, pass both @filename and @lineno, which
 * will be used to output the warning message using the usual logging
 * functions.
 *
 * To copy the token into another string, collapsing any newlines and
 * surrounding whitespace to a single space, pass @dest which should be
 * pre-allocated to the right size (obtained by calling this function
 * with NULL).
 *
 * If you also want quotes to be removed and escaped characters to be
 * replaced with the character itself, set @dequote to TRUE.
 *
 * Returns: the length of the token as it was/would be copied into @dest.
 **/
ssize_t
nih_config_next_token (const char *filename,
		       ssize_t    *lineno,
		       const char *file,
		       ssize_t     len,
		       ssize_t    *pos,
		       char       *dest,
		       const char *delim,
		       int         dequote)
{
	ssize_t  ws = 0, nlws = 0, qc = 0, i = 0, p, ret;
	int      slash = FALSE, quote = 0, nl = FALSE;

	nih_assert ((filename == NULL) || (lineno != NULL));
	nih_assert (file != NULL);
	nih_assert (len > 0);
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


	/* A trailing slash on the end of the file makes no sense, we'll
	 * assume they intended there to be a newline after it and ignore
	 * the character by treating it as whitespace.
	 */
	if (slash) {
		if (filename)
			nih_warn ("%s:%zi: %s", filename, *lineno,
				  _("ignored trailing slash"));

		ws++;
	}

	/* Leaving quotes open is generally bad, close it at the last
	 * piece of whitespace (ie. do nothing :p)
	 */
	if (quote) {
		if (filename)
			nih_warn ("%s:%zi: %s", filename, *lineno,
				  _("unterminated quoted string"));
	}


	/* The return value is the length of the token with any newlines and
	 * surrounding whitespace converted to a single character and any
	 * trailing whitespace removed.
	 *
	 * The actual end of the text read is returned in *pos.
	 */
	ret = p - (pos ? *pos : 0) - ws - nlws - qc;
	if (pos)
		*pos = p;

	return ret;
}

/**
 * nih_config_next_arg:
 * @parent: parent of returned argument,
 * @filename: name of file being parsed,
 * @lineno: line number,
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file.
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
 * If you want warnings to be output, pass both @filename and @lineno, which
 * will be used to output the warning message using the usual logging
 * functions.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned block will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: the argument found or NULL if there wasn't one at this position.
 **/
char *
nih_config_next_arg (const void *parent,
		     const char *filename,
		     ssize_t    *lineno,
		     const char *file,
		     ssize_t     len,
		     ssize_t    *pos)
{
	ssize_t  p, arg_start, arg_len, arg_end;
	char    *arg;

	nih_assert ((filename == NULL) || (lineno != NULL));
	nih_assert (file != NULL);
	nih_assert (len > 0);

	p = (pos ? *pos : 0);
	arg_start = p;
	arg_len = nih_config_next_token (filename, lineno, file, len,
					 &p, NULL, CNLWS, TRUE);
	arg_end = p;

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

	if (pos)
		*pos = p;


	/* If there's nothing to copy, bail out now */
	if (! arg_len)
		return NULL;

	/* Copy in the new token */
	NIH_MUST (arg = nih_alloc (parent, arg_len + 1));

	nih_config_next_token (NULL, NULL,
			       file + arg_start, arg_end - arg_start,
			       NULL, arg, CNLWS, TRUE);

	return arg;
}

/**
 * nih_config_next_line:
 * @filename: name of file being parsed,
 * @lineno: line number,
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file.
 *
 * Skips to the end of the current line in @file.
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
 * If you want warnings to be output, pass both @filename and @lineno, which
 * will be used to output the warning message using the usual logging
 * functions.
 **/
void
nih_config_next_line (const char *filename,
		      ssize_t    *lineno,
		      const char *file,
		      ssize_t     len,
		      ssize_t    *pos)
{
	nih_assert ((filename == NULL) || (lineno != NULL));
	nih_assert (file != NULL);
	nih_assert (len > 0);
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
 * nih_config_parse_args:
 * @parent: parent of returned array,
 * @filename: name of file being parsed,
 * @lineno: line number,
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file.
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
 * If you want warnings to be output, pass both @filename and @lineno, which
 * will be used to output the warning message using the usual logging
 * functions.
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
 * Returns: the list of arguments found.
 **/
char **
nih_config_parse_args (const void *parent,
		       const char *filename,
		       ssize_t    *lineno,
		       const char *file,
		       ssize_t     len,
		       ssize_t    *pos)
{
	char    **args;
	ssize_t   p;
	size_t    nargs;

	nih_assert ((filename == NULL) || (lineno != NULL));
	nih_assert (file != NULL);
	nih_assert (len > 0);

	/* Begin with an empty array */
	NIH_MUST (args = nih_alloc (parent, sizeof (char *)));
	args[0] = NULL;
	nargs = 0;

	/* Loop through the arguments until we hit a comment or newline */
	p = (pos ? *pos : 0);
	while ((p < len) && (! strchr (CNL, file[p]))) {
		char    **new_args, *arg;

		arg = nih_config_next_arg (args, filename, lineno,
					   file, len, &p);
		if (! arg)
			continue;

		/* Extend the array and allocate room for the args */
		NIH_MUST (new_args = nih_realloc (args, parent,
						  (sizeof (char *)
						   * (nargs + 2))));

		args = new_args;
		args[nargs++] = arg;
		args[nargs] = NULL;

	}

	nih_config_next_line (filename, lineno, file, len, &p);

	if (pos)
		*pos = p;

	return args;
}

/**
 * nih_config_parse_command:
 * @parent: parent of returned string,
 * @filename: name of file being parsed,
 * @lineno: line number,
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file.
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
 * If you want warnings to be output, pass both @filename and @lineno, which
 * will be used to output the warning message using the usual logging
 * functions.
 *
 * The command is returned as a string allocated with nih_alloc().
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned block will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: the command string found or NULL if one was not present.
 **/
char *
nih_config_parse_command (const void *parent,
			  const char *filename,
			  ssize_t    *lineno,
			  const char *file,
			  ssize_t     len,
			  ssize_t    *pos)
{
	char    *cmd;
	ssize_t  p, cmd_start, cmd_len, cmd_end;

	nih_assert ((filename == NULL) || (lineno != NULL));
	nih_assert (file != NULL);
	nih_assert (len > 0);

	/* Find the length of string up to the first unescaped comment
	 * or newline.
	 */
	p = (pos ? *pos : 0);
	cmd_start = p;
	cmd_len = nih_config_next_token (filename, lineno, file, len,
					 &p, NULL, CNL, FALSE);
	cmd_end = p;

	nih_config_next_line (filename, lineno, file, len, &p);

	if (pos)
		*pos = p;

	/* If there's nothing to copy, bail out now */
	if (! cmd_len)
		return NULL;


	/* Now copy the string into the destination. */
	NIH_MUST (cmd = nih_alloc (parent, cmd_len + 1));
	nih_config_next_token (NULL, NULL,
			       file + cmd_start, cmd_end - cmd_start,
			       NULL, cmd, CNL, FALSE);

	return cmd;
}


/**
 * nih_config_parse_block:
 * @parent: parent of returned string,
 * @filename: name of file being parsed,
 * @lineno: line number,
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
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
 * If you want warnings to be output, pass both @filename and @lineno, which
 * will be used to output the warning message using the usual logging
 * functions.
 *
 * The block is returned as a string allocated with nih_alloc().
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned block will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: the text contained within the block.
 **/
char *
nih_config_parse_block (const void *parent,
			const char *filename,
			ssize_t    *lineno,
			const char *file,
			ssize_t     len,
			ssize_t    *pos,
			const char *type)
{
	char    *block;
	ssize_t  p, sh_start, sh_end, sh_len, ws;
	int      lines;

	nih_assert ((filename == NULL) || (lineno != NULL));
	nih_assert (file != NULL);
	nih_assert (len > 0);
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

	while ((sh_end = nih_config_block_end (lineno, file, len,
					       &p, type)) < 0) {
		ssize_t line_start;

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

		nih_config_next_line (filename, lineno, file, len, &p);

		if (p >= len) {
			sh_end = p;
			if (filename)
				nih_warn ("%s:%zi: %s", filename, *lineno,
					  _("end of block expected"));
			break;
		}
	}

	if (pos)
		*pos = p;


	/*
	 * Copy the fragment into a string, removing common whitespace from
	 * the start.  We can be less strict here because we already know
	 * the contents, etc.
	 */

	sh_len = sh_end - sh_start - (ws * lines);
	NIH_MUST (block = nih_alloc (parent, sh_len + 1));
	block[0] = '\0';

	p = sh_start;
	while (p < sh_end) {
		size_t line_start;

		p += ws;
		line_start = p;

		while (file[p++] != '\n')
			;

		strncat (block, file + line_start, p - line_start);
	}

	return block;
}

/**
 * nih_config_block_end:
 * @lineno: line number,
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
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
nih_config_block_end (ssize_t    *lineno,
		      const char *file,
		      ssize_t     len,
		      ssize_t    *pos,
		      const char *type)
{
	ssize_t p, end;

	nih_assert (file != NULL);
	nih_assert (len > 0);
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
 * @filename: name of file being parsed,
 * @lineno: line number,
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
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
 * If you want warnings to be output, pass both @filename and @lineno, which
 * will be used to output the warning message using the usual logging
 * functions.
 *
 * A warning is output if no stanza is found, and the position is moved
 * past the end of the line.  This may result in inconsistent parsing.
 *
 * Returns: return value from handler or zero if no handler found.
 **/
int
nih_config_parse_stanza (const char      *filename,
			 ssize_t         *lineno,
			 const char      *file,
			 ssize_t          len,
			 ssize_t         *pos,
			 NihConfigStanza *stanzas,
			 void            *data)
{
	NihConfigStanza *stanza;
	char            *name;
	ssize_t          p;
	int              ret = 0;

	nih_assert ((filename == NULL) || (lineno != NULL));
	nih_assert (file != NULL);
	nih_assert (len > 0);
	nih_assert (stanzas != NULL);

	p = (pos ? *pos : 0);

	/* Get the next dequoted argument from the file */
	name = nih_config_next_arg (NULL, filename, lineno, file, len, &p);
	if (! name)
		return 0;

	/* Lookup the stanza for it */
	stanza = nih_config_get_stanza (name, stanzas);
	if (stanza) {
		ret = stanza->handler (data, stanza, filename, lineno,
				       file, len, &p);
	} else {
		if (filename)
			nih_warn ("%s:%zi: %s: %s", filename, *lineno,
				  _("ignored unknown stanza"), name);

		nih_config_next_line (filename, lineno, file, len, &p);
	}

	nih_free (name);

	if (pos)
		*pos = p;

	return ret;
}


/**
 * nih_config_parse_file:
 * @filename: name of file being parsed,
 * @lineno: line number,
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
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
 * If you want warnings to be output, pass both @filename and @lineno, which
 * will be used to output the warning message using the usual logging
 * functions.
 **/
void
nih_config_parse_file (const char      *filename,
		       ssize_t         *lineno,
		       const char      *file,
		       ssize_t          len,
		       ssize_t         *pos,
		       NihConfigStanza *stanzas,
		       void            *data)
{
	ssize_t p;

	nih_assert ((filename == NULL) || (lineno != NULL));
	nih_assert (file != NULL);
	nih_assert (len > 0);
	nih_assert (stanzas != NULL);

	p = (pos ? *pos : 0);

	while (p < len) {
		/* Skip initial whitespace */
		while ((p < len) && strchr (WS, file[p]))
			p++;

		/* Skip over comment until end of line */
		if ((p < len) && (file[p] == '#'))
			while ((p < len) && (file[p] != '\n'))
				p++;

		/* Ignore blank lines */
		if ((p < len) && (file[p] == '\n')) {
			if (lineno)
				(*lineno)++;

			p++;
			continue;
		}

		/* Must have a stanza, parse it */
		if (p < len)
			nih_config_parse_stanza (filename, lineno, file, len,
						 &p, stanzas, data);
	}

	if (pos)
		*pos = p;
}

/**
 * nih_config_parse:
 * @filename: name of file to parse,
 * @stanzas: table of stanza handlers,
 * @data: pointer to pass to stanza handler.
 *
 * Maps @filename into memory and them parses configuration lines from it
 * using nih_config_parse_file().
 *
 * Parser errors are only treated as warnings and are output using the
 * usual logging functions, prefixed with both the filename and line number
 * that the error was found.
 *
 * The only raised errors from this function are those caused by failure
 * to map or unmap the file.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
nih_config_parse (const char      *filename,
		  NihConfigStanza *stanzas,
		  void            *data)
{
	const char *file;
	ssize_t     len, pos, lineno;

	nih_assert (filename != NULL);

	file = nih_file_map (filename, O_RDONLY | O_NOCTTY, (size_t *)&len);
	if (! file)
		return -1;

	pos = 0;
	lineno = 1;
	nih_config_parse_file (filename, &lineno, file, len, &pos,
			       stanzas, data);

	if (nih_file_unmap ((void *)file, len) < 0)
		return -1;

	return 0;
}
