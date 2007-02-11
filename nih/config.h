/* libnih
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

#ifndef NIH_CONFIG_H
#define NIH_CONFIG_H

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

ssize_t   nih_config_token           (const char *file, size_t len,
				      size_t *pos, size_t *lineno, char *dest,
				      const char *delim, int dequote)
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
