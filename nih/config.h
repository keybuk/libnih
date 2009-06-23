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

#ifndef NIH_CONFIG_H
#define NIH_CONFIG_H

/**
 * Implements a flexible configuration parser based on keyword stanzas and zero
 * or more arguments, handling such matters as quotation, whitespace and
 * commands for you.
 *
 * You describe the stanzas with an array of NihConfigStanza members,
 * each is handled by a function that receives a number of arguments
 * referencing the current position within the file being parsed.
 *
 * The function may then call any of the parsing functions to parse its
 * arguments, consuming as much of the following file as it wishes.  Most
 * will call nih_config_parse_args() to do the hard work.
 *
 * Configuration can be parsed as a file with nih_config_parse_file() or
 * as a string with nih_config_parse().
 **/

#include <sys/types.h>

#include <nih/macros.h>


/**
 * NihConfigHandler:
 * @data: data passed to parser,
 * @stanza: stanza found,
 * @filename: name of file being parsed,
 * @lineno: line number,
 * @file: file or string to parse,
 * @len: length of @file,
 * @pos: offset within @file.
 *
 * A config handler is a function that is called when @stanza is found
 * in a configuration file.
 *
 * @file may be a memory mapped file, in which case @pos is given
 * as the offset within that the stanza's arguments begin, and @len will
 * be the length of the file as a whole.
 *
 * @pos must be updated to point to the next stanza in the configuration
 * file, past whatever terminator is used for the one being parsed.
 *
 * If @lineno is not NULL, it contains the current line number and must be
 * incremented each time a new line is discovered in the file.
 *
 * (These things are taken care of for you if you use the nih_config_*
 * functions).
 *
 * If you encounter errors, you should use the usual logging functions to
 * output warnings using both @filename and @lineno, but only if @filename
 * is not NULL.
 *
 * Returns: zero on success, negative value on raised error.
 **/
typedef struct nih_config_stanza NihConfigStanza;
typedef int (*NihConfigHandler) (void *data, NihConfigStanza *stanza,
				 const char *file, size_t len, size_t *pos,
				 size_t *lineno);


/**
 * NihConfigStanza:
 * @name: stanza name,
 * @handler: function to call.
 *
 * This structure defines a configuration file stanza, when a stanza
 * called @name is found within a configuration file, @handler will be
 * called from a position after the stanza and any following whitespace.
 **/
struct nih_config_stanza {
	char             *name;
	NihConfigHandler  handler;
};


/**
 * NIH_CONFIG_LAST:
 *
 * This macro may be used as the last stanza in the list to avoid typing
 * all those NULLs yourself.
 **/
#define NIH_CONFIG_LAST { NULL, NULL }


/**
 * NIH_CONFIG_WS:
 *
 * Definition of what characters we consider whitespace.
 **/
#define NIH_CONFIG_WS " \t\r"

/**
 * NIH_CONFIG_CNL:
 *
 * Definition of what characters nominally end a line; a comment start
 * character or a newline.
 **/
#define NIH_CONFIG_CNL "#\n"

/**
 * NIH_CONFIG_CNLWS:
 *
 * Defintion of what characters nominally separate tokens.
 **/
#define NIH_CONFIG_CNLWS " \t\r#\n"


NIH_BEGIN_EXTERN

int       nih_config_has_token       (const char *file, size_t len,
				      size_t *pos, size_t *lineno);

int       nih_config_token           (const char *file, size_t len,
				      size_t *pos, size_t *lineno, char *dest,
				      const char *delim, int dequote,
				      size_t *toklen)
	__attribute__ ((warn_unused_result));
char *    nih_config_next_token      (const void *parent, const char *file,
				      size_t len, size_t *pos, size_t *lineno,
				      const char *delim, int dequote)
	__attribute__ ((warn_unused_result, malloc));
char *    nih_config_next_arg        (const void *parent, const char *file,
				      size_t len, size_t *pos, size_t *lineno)
	__attribute__ ((warn_unused_result, malloc));
void      nih_config_next_line       (const char *file, size_t len,
				      size_t *pos, size_t *lineno);

void      nih_config_skip_whitespace (const char *file, size_t len,
				      size_t *pos, size_t *lineno);
int       nih_config_skip_comment    (const char *file, size_t len,
				      size_t *pos, size_t *lineno)
	__attribute__ ((warn_unused_result));

char **   nih_config_parse_args      (const void *parent, const char *file,
				      size_t len, size_t *pos, size_t *lineno)
	__attribute__ ((warn_unused_result, malloc));
char *    nih_config_parse_command   (const void *parent, const char *file,
				      size_t len, size_t *pos, size_t *lineno)
	__attribute__ ((warn_unused_result, malloc));

char *    nih_config_parse_block     (const void *parent, const char *file,
				      size_t len, size_t *pos, size_t *lineno,
				      const char *type)
	__attribute__ ((warn_unused_result, malloc));
int       nih_config_skip_block      (const char *file, size_t len,
				      size_t *lineno, size_t *pos,
				      const char *type, size_t *endpos)
	__attribute__ ((warn_unused_result));

int       nih_config_parse_stanza    (const char *file, size_t len,
				      size_t *pos, size_t *lineno,
				      NihConfigStanza *stanzas, void *data)
	__attribute__ ((warn_unused_result));

int       nih_config_parse_file      (const char *file, size_t len,
				      size_t *pos, size_t *lineno,
				      NihConfigStanza *stanzas, void *data)
	__attribute__ ((warn_unused_result));
int       nih_config_parse           (const char *filename, size_t *pos,
				      size_t *lineno, NihConfigStanza *stanzas,
				      void *data)
	__attribute__ ((warn_unused_result));

NIH_END_EXTERN

#endif /* NIH_CONFIG_H */
