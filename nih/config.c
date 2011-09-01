/* libnih
 *
 * config.c - configuration file parsing
 *
 * Copyright © 2011 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2011 Canonical Ltd.
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


#include <sys/types.h>

#include <limits.h>
#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/file.h>
#include <nih/config.h>
#include <nih/logging.h>
#include <nih/error.h>
#include <nih/errors.h>


/* Prototypes for static functions */
static int              nih_config_block_end  (const char *file, size_t len,
					       size_t *lineno, size_t *pos,
					       const char *type,
					       size_t *endpos)
	__attribute__ ((warn_unused_result));
static NihConfigStanza *nih_config_get_stanza (const char *name,
					       NihConfigStanza *stanzas);


/**
 * nih_config_has_token:
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
 * @lineno: line number.
 *
 * Checks the current position in @file to see whether it has a parseable
 * token at this position; ie. we're not at the end of file, and the
 * current character is neither a comment or newline character.
 *
 * If this returns FALSE, it's normal to call nih_config_skip_comment()
 * to move to the next parseable point and check again.
 *
 * @file may be a memory mapped file, in which case @pos should be given
 * as the offset within and @len should be the length of the file as a
 * whole.
 *
 * @pos is used as the offset within @file to begin, otherwise the start
 * is assumed.
 *
 * Returns: TRUE if the current character is before the end of file and
 * is neither a comment or newline, FALSE otherwise.
 **/
int
nih_config_has_token (const char *file,
		      size_t      len,
		      size_t     *pos,
		      size_t     *lineno)
{
	size_t p;

	nih_assert (file != NULL);

	p = (pos ? *pos : 0);
	if ((p < len) && (! strchr (NIH_CONFIG_CNL, file[p]))) {
		return TRUE;
	} else {
		return FALSE;
	}
}


/**
 * nih_config_token:
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
 * @lineno: line number,
 * @dest: destination to copy to,
 * @delim: characters to stop on,
 * @dequote: remove quotes and escapes.
 * @toklen: pointer to store token length in.
 *
 * Parses a single token from @file which is stopped when any character
 * in @delim is encountered outside of a quoted string and not escaped
 * using a backslash.  The length of the parsed token is stored in @toklen
 * if given.
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
 * Returns: zero on success, negative value on raised error.
 **/
int
nih_config_token (const char *file,
		  size_t      len,
		  size_t     *pos,
		  size_t     *lineno,
		  char       *dest,
		  const char *delim,
		  int         dequote,
		  size_t     *toklen)
{
	size_t  p, ws = 0, nlws = 0, qc = 0, i = 0;
	int     slash = FALSE, quote = 0, nl = FALSE, ret = 0;

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
			} else if ((file[p] == '\\')
				   || strchr (NIH_CONFIG_WS, file[p])) {
				extra++;
				if (dequote)
					qc++;
			} else if (dest) {
				dest[i++] = '\\';
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
			} else if (strchr (NIH_CONFIG_WS, file[p])) {
				ws++;
				continue;
			}
		} else if ((file[p] == '\"') || (file[p] == '\'')) {
			quote = file[p];
			isq = TRUE;
		} else if (strchr (delim, file[p])) {
			break;
		} else if (strchr (NIH_CONFIG_WS, file[p])) {
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


	/* The token length we return is the length of the token with any
	 * newlines and surrounding whitespace converted to a single
	 * character and any trailing whitespace removed.
	 *
	 * The actual end of the text read is returned in *pos.
	 */
	if (toklen)
		*toklen = p - (pos ? *pos : 0) - ws - nlws - qc;

finish:
	if (pos)
		*pos = p;

	return ret;
}

/**
 * nih_config_next_token:
 * @parent: parent object for returned token,
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
 * @lineno: line number,
 * @delim: characters to stop on,
 * @dequote: remove quotes and escapes.
 *
 * Extracts a single token from @file which is stopped when any character
 * in @delim is encountered outside of a quoted string and not escaped
 * using a backslash.  If @delim contains any whitespace character, then
 * all whitespace after the token is also consumed, but not returned,
 * including that with escaped newlines within it.
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
 * If you also want quotes to be removed and escaped characters to be
 * replaced with the character itself, set @dequote to TRUE.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned token.  When all parents
 * of the returned token are freed, the returned token will also be
 * freed.
 *
 * Returns: the token found or NULL on raised error.
 **/
char *
nih_config_next_token (const void *parent,
		       const char *file,
		       size_t      len,
		       size_t     *pos,
		       size_t     *lineno,
		       const char *delim,
		       int         dequote)
{
	size_t  p, arg_start, arg_len, arg_end;
	char   *arg = NULL;

	nih_assert (file != NULL);

	p = (pos ? *pos : 0);
	arg_start = p;
	if (nih_config_token (file, len, &p, lineno, NULL, delim, dequote,
			      &arg_len) < 0)
		goto finish;

	arg_end = p;
	if (! arg_len) {
		nih_error_raise (NIH_CONFIG_EXPECTED_TOKEN,
				 _(NIH_CONFIG_EXPECTED_TOKEN_STR));
		goto finish;
	}

	nih_config_skip_whitespace (file, len, &p, lineno);

	/* Copy in the new token */
	arg = nih_alloc (parent, arg_len + 1);
	if (! arg)
		nih_return_system_error (NULL);

	if (nih_config_token (file + arg_start, arg_end - arg_start, NULL,
			      NULL, arg, delim, dequote, NULL) < 0)
		goto finish;

finish:
	if (pos)
		*pos = p;

	return arg;
}

/**
 * nih_config_next_arg:
 * @parent: parent object for returned argument,
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
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned argument.  When all parents
 * of the returned argument are freed, the returned argument will also be
 * freed.
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
	nih_assert (file != NULL);

	return nih_config_next_token (parent, file, len, pos, lineno,
				      NIH_CONFIG_CNLWS, TRUE);
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
 * nih_config_skip_whitespace:
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
 * @lineno: line number.
 *
 * Skips an amount of whitespace and finds either the next token or the end
 * of the current line in @file.  Escaped newlines within the whitespace
 * are treated as whitespace.
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
nih_config_skip_whitespace (const char *file,
			    size_t      len,
			    size_t     *pos,
			    size_t     *lineno)
{
	nih_assert (file != NULL);
	nih_assert (pos != NULL);

	/* Skip any amount of whitespace between them, we also need to
	 * detect an escaped newline here.
	 */
	while (*pos < len) {
		if (file[*pos] == '\\') {
			/* Escape character, only continue scanning if
			 * the next character is newline
			 */
			if ((len - *pos > 1) && (file[*pos + 1] == '\n')) {
				(*pos)++;
			} else {
				break;
			}
		} else if (! strchr (NIH_CONFIG_WS, file[*pos])) {
			break;
		}

		if (file[*pos] == '\n')
			if (lineno)
				(*lineno)++;

		/* Whitespace characer */
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

	if (nih_config_has_token (file, len, pos, lineno)) {
		nih_error_raise (NIH_CONFIG_UNEXPECTED_TOKEN,
				 _(NIH_CONFIG_UNEXPECTED_TOKEN_STR));
		return -1;
	}

	nih_config_next_line (file, len, pos, lineno);

	return 0;
}


/**
 * nih_config_parse_args:
 * @parent: parent object for returned array,
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
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned array.  When all parents
 * of the returned array are freed, the returned array will also be
 * freed.
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
	nargs = 0;
	args = nih_str_array_new (parent);
	if (! args)
		nih_return_system_error (NULL);

	/* Loop through the arguments until we hit a comment or newline */
	p = (pos ? *pos : 0);
	while (nih_config_has_token (file, len, &p, lineno)) {
		char *arg;

		arg = nih_config_next_arg (args, file, len, &p, lineno);
		if (! arg) {
			nih_free (args);
			args = NULL;
			goto finish;
		}

		if (! nih_str_array_addp (&args, parent, &nargs, arg)) {
			nih_error_raise_system ();
			nih_free (args);
			return NULL;
		}
	}

	/* nih_config_has_token has returned FALSE, we must be either past
	 * the end of the file, or at a comment or newline.
	 */
	if (nih_config_skip_comment (file, len, &p, lineno) < 0)
		nih_assert_not_reached ();

finish:
	if (pos)
		*pos = p;

	return args;
}

/**
 * nih_config_parse_command:
 * @parent: parent object for returned string,
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
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: the newly allocated command found or NULL on raised error.
 **/
char *
nih_config_parse_command (const void *parent,
			  const char *file,
			  size_t      len,
			  size_t     *pos,
			  size_t     *lineno)
{
	char   *cmd = NULL;
	size_t  p, cmd_start, cmd_len, cmd_end;

	nih_assert (file != NULL);

	/* Find the length of string up to the first unescaped comment
	 * or newline.
	 */
	p = (pos ? *pos : 0);
	cmd_start = p;
	if (nih_config_token (file, len, &p, lineno, NULL,
			      NIH_CONFIG_CNL, FALSE, &cmd_len) < 0)
		goto finish;

	cmd_end = p;

	/* nih_config_token will eat up to the end of the file, a comment
	 * or a newline; so this must always succeed.
	 */
	if (nih_config_skip_comment (file, len, &p, lineno) < 0)
		nih_assert_not_reached ();

	/* Now copy the string into the destination. */
	cmd = nih_alloc (parent, cmd_len + 1);
	if (! cmd)
		nih_return_system_error (NULL);

	if (nih_config_token (file + cmd_start, cmd_end - cmd_start, NULL,
			      NULL, cmd, NIH_CONFIG_CNL, FALSE, NULL) < 0)
		goto finish;

finish:
	if (pos)
		*pos = p;

	return cmd;
}


/**
 * nih_config_parse_block:
 * @parent: parent object for returned string,
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
 * @lineno: line number,
 * @type: block identifier.
 *
 * Extracts a block of text from @line, stopping when the phrase "end @type"
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
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
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
	size_t   p, pp, sh_start, sh_len, sh_end, ws;
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
	sh_end = 0;
	ws = 0;
	lines = 0;

	while (! nih_config_block_end (file, len, &p, lineno, type, &sh_end)) {
		size_t line_start;

		lines++;
		line_start = p;

		if (lines == 1) {
			/* Count whitespace on the first line */
			while ((p < len) && strchr (NIH_CONFIG_WS, file[p]))
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
	block = nih_alloc (parent, sh_len + 1);
	if (! block)
		nih_return_system_error (NULL);

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
 * nih_config_skip_block:
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
 * @lineno: line number,
 * @type: block identifier,
 * @endpos: pointer to end of block.
 *
 * Skips over a block of text from @file, stopping when the phrase
 * "end @type" is encountered without any quotes or blackslash escaping
 * within it.
 *
 * @file may be a memory mapped file, in which case @pos should be given
 * as the offset within and @len should be the length of the file as a
 * whole.
 *
 * If @pos is given then it will be used as the offset within @file to
 * begin (otherwise the start is assumed), and will be updated to point
 * past the end of the block and block marker or the end of the file.
 *
 * Either @file or @pos should point to the start of the block, after the
 * opening stanza, rather than the start of the stanza that opens it.
 *
 * If @lineno is given it will be incremented each time a new line is
 * discovered in the file.
 *
 * @endpos will be set to the end of the block and the start of the block
 * marker, this is useful for determining the length of the block skipped,
 * to parse it for example.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
nih_config_skip_block (const char *file,
		       size_t      len,
		       size_t     *pos,
		       size_t     *lineno,
		       const char *type,
		       size_t     *endpos)
{
	size_t p;
	int    ret = 0;

	nih_assert (file != NULL);
	nih_assert (type != NULL);

	p = (pos ? *pos : 0);

	while (! nih_config_block_end (file, len, &p, lineno, type, endpos)) {
		nih_config_next_line (file, len, &p, lineno);

		if (p >= len) {
			nih_error_raise (NIH_CONFIG_UNTERMINATED_BLOCK,
					 _(NIH_CONFIG_UNTERMINATED_BLOCK_STR));
			ret = -1;
			goto finish;
		}
	}

finish:
	if (pos)
		*pos = p;

	return ret;
}

/**
 * nih_config_block_end:
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file,
 * @lineno: line number,
 * @type: block identifier,
 * @endpos: pointer to end of block.
 *
 * Determines whether the current line contains an end of block marker,
 * and if so, sets @endpos to the end  of the block.
 *
 * @file may be a memory mapped file, in which case @pos should be given
 * as the offset within and @len should be the length of the file.  @pos
 * will be updated to point past the end of the block and the end block
 * marker or the end of the file.
 *
 * @lineno will be incremented each time a new line is discovered.
 *
 * Returns: TRUE if at the end of the block, FALSE otherwise.
 **/
static int
nih_config_block_end (const char *file,
		      size_t      len,
		      size_t     *pos,
		      size_t     *lineno,
		      const char *type,
		      size_t     *endpos)
{
	size_t p;

	nih_assert (file != NULL);
	nih_assert (pos != NULL);
	nih_assert (type != NULL);

	p = *pos;

	/* Skip initial whitespace */
	while ((p < len) && strchr (NIH_CONFIG_WS, file[p]))
		p++;

	/* Check the first word (check we have at least 4 chars because of
	 * the need for whitespace immediately after)
	 */
	if ((len - p < 4) || strncmp (file + p, "end", 3))
		return FALSE;

	/* Must be whitespace after */
	if (file[p + 3] && ! strchr (NIH_CONFIG_WS, file[p + 3]))
		return FALSE;

	/* Find the second word */
	p += 3;
	while ((p < len) && strchr (NIH_CONFIG_WS, file[p]))
		p++;

	/* Check the second word */
	if ((len - p < strlen (type))
	    || strncmp (file + p, type, strlen (type)))
		return FALSE;

	/* May be followed by whitespace */
	p += strlen (type);
	while ((p < len) && strchr (NIH_CONFIG_WS, file[p]))
		p++;

	/* May be a comment, in which case eat up to the newline
	 */
	if ((p < len) && (file[p] == '#')) {
		while ((p < len) && (file[p] != '\n'))
			p++;
	}

	/* Should be end of string, or a newline */
	if ((p < len) && (file[p] != '\n'))
		return FALSE;

	/* Point past the new line */
	if (p < len) {
		if (lineno)
			(*lineno)++;
		p++;
	}

	/* Set endpos to the beginning of the line (which is the end of the
	 * script) but update pos to point past this line.
	 */
	if (endpos)
		*endpos = *pos;
	*pos = p;

	return TRUE;
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
 * If any entry exists with the stanza name "", this is returned instead
 * of NULL if no specific entry is found.
 *
 * Returns: stanza found or NULL if no handler for @name.
 **/
static NihConfigStanza *
nih_config_get_stanza (const char      *name,
		       NihConfigStanza *stanzas)
{
	NihConfigStanza *stanza, *catch = NULL;

	for (stanza = stanzas; (stanza->name && stanza->handler); stanza++) {
		if (! strlen (stanza->name))
			catch = stanza;

		if (! strcmp (stanza->name, name))
			return stanza;
	}

	return catch;
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
	nih_local char  *name = NULL;
	size_t           p;
	int              ret = -1;

	nih_assert (file != NULL);
	nih_assert (stanzas != NULL);

	p = (pos ? *pos : 0);

	/* Get the next dequoted argument from the file */
	name = nih_config_next_token (NULL, file, len, &p, lineno,
				      NIH_CONFIG_CNLWS, FALSE);
	if (! name)
		goto finish;

	/* Lookup the stanza for it */
	stanza = nih_config_get_stanza (name, stanzas);
	if (! stanza)
		nih_return_error (-1, NIH_CONFIG_UNKNOWN_STANZA,
				  _(NIH_CONFIG_UNKNOWN_STANZA_STR));

	ret = stanza->handler (data, stanza, file, len, &p, lineno);

finish:
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
		while ((p < len) && strchr (NIH_CONFIG_WS, file[p]))
			p++;

		/* Skip lines with only comments in them; because has_token
		 * returns FALSE we know we're either past the end of the
		 * file, at a comment, or a newline.
		 */
		if (! nih_config_has_token (file, len, &p, lineno)) {
			if (nih_config_skip_comment (file, len,
						     &p, lineno) < 0)
				nih_assert_not_reached ();

			continue;
		}

		/* Must have a stanza, parse it */
		if (nih_config_parse_stanza (file, len, &p, lineno,
						     stanzas, data) < 0)
			goto finish;
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
 * Reads @filename into memory and them parses configuration lines from it
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
	nih_local char *file = NULL;
	size_t          len;
	int             ret;

	nih_assert (filename != NULL);

	file = nih_file_read (NULL, filename, &len);
	if (! file)
		return -1;

	if (lineno)
		*lineno = 1;

	ret = nih_config_parse_file (file, len, pos, lineno, stanzas, data);

	return ret;
}
