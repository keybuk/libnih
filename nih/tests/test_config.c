/* libnih
 *
 * test_config.c - test suite for nih/config.c
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

#include <nih/test.h>

#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/config.h>
#include <nih/main.h>
#include <nih/error.h>
#include <nih/errors.h>


void
test_has_token (void)
{
	char   buf[1024];
	size_t pos;
	int    ret;

	TEST_FUNCTION ("nih_config_has_token");
	strcpy (buf, "this is a test # comment\n");

	/* Check that an ordinary token character at the start of the line
	 * causes the function to return TRUE.
	 */
	TEST_FEATURE ("with token at start of string");
	ret = nih_config_has_token (buf, strlen (buf), NULL, NULL);

	TEST_TRUE (ret);


	/* Check that an ordinary token inside the string causes the function
	 * to return TRUE.
	 */
	TEST_FEATURE ("with token inside string");
	pos = 5;
	ret = nih_config_has_token (buf, strlen (buf), &pos, NULL);

	TEST_TRUE (ret);


	/* Check that a piece of whitespace causes the function to return TRUE.
	 */
	TEST_FEATURE ("with whitespace");
	pos = 7;
	ret = nih_config_has_token (buf, strlen (buf), &pos, NULL);

	TEST_TRUE (ret);


	/* Check that a comment character causes the function to return FALSE.
	 */
	TEST_FEATURE ("with start of comment");
	pos = 15;
	ret = nih_config_has_token (buf, strlen (buf), &pos, NULL);

	TEST_FALSE (ret);


	/* Check that a newline character causes the function to return FALSE.
	 */
	TEST_FEATURE ("with newline");
	pos = 24;
	ret = nih_config_has_token (buf, strlen (buf), &pos, NULL);

	TEST_FALSE (ret);


	/* Check that the end of file causes the function to return FALSE.
	 */
	TEST_FEATURE ("at end of file");
	pos = 25;
	ret = nih_config_has_token (buf, strlen (buf), &pos, NULL);

	TEST_FALSE (ret);
}


void
test_token (void)
{
	char      buf[1024], dest[1024];
	size_t    pos, lineno, len;
	int       ret;
	NihError *err;

	TEST_FUNCTION ("nih_config_token");
	program_name = "test";

	/* Check that we can obtain the length of the first simple token
	 * in a string, and that the position is updated past it.  The
	 * length of the token should be returned.
	 */
	TEST_FEATURE ("with token at start of string");
	strcpy (buf, "this is a test");
	pos = 0;

	len = 0;
	ret = nih_config_token (buf, strlen (buf), &pos, NULL,
				NULL, " ", FALSE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 4);
	TEST_EQ (pos, 4);


	/* Check that we can obtain a length of a token that entirely fills
	 * the remainder of the file.
	 */
	TEST_FEATURE ("with token filling string");
	strcpy (buf, "wibble");
	pos = 0;
	len = 0;
	ret = nih_config_token (buf, strlen (buf), &pos, NULL,
				NULL, " ", FALSE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 6);
	TEST_EQ (pos, 6);


	/* Check that we can extract a token from the string and have it
	 * copied into our destination buffer.
	 */
	TEST_FEATURE ("with token to extract");
	strcpy (buf, "this is a test");
	ret = nih_config_token (buf, strlen (buf), NULL, NULL,
				dest, " ", FALSE, NULL);

	TEST_EQ (ret, 0);
	TEST_EQ_STR (dest, "this");


	/* Check that we can obtain the length of a simple token inside the
	 * string, and the the position is updated past it.
	 */
	TEST_FEATURE ("with token inside string");
	pos = 5;
	len = 0;
	ret = nih_config_token (buf, strlen (buf), &pos, NULL,
				NULL, " ", FALSE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 2);
	TEST_EQ (pos, 7);


	/* Check that we can obtain the length of a token that contains
	 * double quotes around the delimeter, the length should include
	 * the quoted part at the quotes.
	 */
	TEST_FEATURE ("with double quotes inside token");
	strcpy (buf, "\"this is a\" test");
	pos = 0;
	len = 0;
	ret = nih_config_token (buf, strlen (buf), &pos, NULL,
				NULL, " ", FALSE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 11);
	TEST_EQ (pos, 11);


	/* Check that we can extract a token that is surrounded by double
	 * quotes, we should still get those.
	 */
	TEST_FEATURE ("with double quotes around token to extract");
	len = 0;
	ret = nih_config_token (buf, strlen (buf), NULL, NULL,
				dest, " ", FALSE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 11);
	TEST_EQ_STR (dest, "\"this is a\"");


	/* Check that we can obtain the length of the quoted portion, with
	 * the quotes removed; the position should still point past it.
	 */
	TEST_FEATURE ("with double quotes and dequoting");
	pos = 0;
	len = 0;
	ret = nih_config_token (buf, strlen (buf), &pos, NULL,
				NULL, " ", TRUE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 9);
	TEST_EQ (pos, 11);


	/* Check that we can extract a quoted token and have the quotes
	 * removed.
	 */
	TEST_FEATURE ("with double quotes and extract with dequoting");
	ret = nih_config_token (buf, strlen (buf), NULL, NULL,
				dest, " ", TRUE, NULL);

	TEST_EQ (ret, 0);
	TEST_EQ_STR (dest, "this is a");


	/* Check that we can obtain the length of a token that contains
	 * single quotes around the delimeter, the length should include
	 * the quoted part at the quotes.
	 */
	TEST_FEATURE ("with single quotes inside token");
	strcpy (buf, "\'this is a\' test");
	pos = 0;
	len = 0;
	ret = nih_config_token (buf, strlen (buf), &pos, NULL,
				NULL, " ", FALSE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 11);
	TEST_EQ (pos, 11);


	/* Check that we can obtain the length of a token that contains
	 * escaped spaces around the delimeter, the length should include
	 * the backslashes.
	 */
	TEST_FEATURE ("with escaped spaces inside token");
	strcpy (buf, "this\\ is\\ a test");
	pos = 0;
	len = 0;
	ret = nih_config_token (buf, strlen (buf), &pos, NULL,
				NULL, " ", FALSE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 11);
	TEST_EQ (pos, 11);


	/* Check that we can extract a token that contains escaped spaces
	 * around the delimiter.
	 */
	TEST_FEATURE ("with escaped spaces within extracted token");
	ret = nih_config_token (buf, strlen (buf), NULL, NULL,
				dest, " ", FALSE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 11);
	TEST_EQ_STR (dest, "this\\ is\\ a");


	/* Check that we can obtain the length of a token that contains
	 * escaped spaces around the delimeter, without the blackslashes.
	 */
	TEST_FEATURE ("with escaped spaces inside token and dequoting");
	pos = 0;
	len = 0;
	ret = nih_config_token (buf, strlen (buf), &pos, NULL,
				NULL, " ", TRUE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 9);
	TEST_EQ (pos, 11);


	/* Check that we can extract a token that contains escaped spaces
	 * around the delimiter, while removing them.
	 */
	TEST_FEATURE ("with escaped spaces within extracted dequoted token");
	len = 0;
	ret = nih_config_token (buf, strlen (buf), NULL, NULL,
				dest, " ", TRUE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 9);
	TEST_EQ_STR (dest, "this is a");


	/* Check that a newline inside a quoted string, and surrounding
	 * whitespace, is treated as a single space character.
	 */
	TEST_FEATURE ("with newline inside quoted string");
	strcpy (buf, "\"this is \n a\" test");
	pos = 0;
	lineno = 1;
	len = 0;
	ret = nih_config_token (buf, strlen (buf), &pos, &lineno,
				NULL, " ", FALSE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 11);
	TEST_EQ (pos, 13);
	TEST_EQ (lineno, 2);


	/* Check that extracting a token with a newline inside a quoted
	 * string only returns a single space for the newline.
	 */
	TEST_FEATURE ("with newline inside extracted quoted string");
	len = 0;
	ret = nih_config_token (buf, strlen (buf), NULL, NULL,
				dest, " ", FALSE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 11);
	TEST_EQ_STR (dest, "\"this is a\"");


	/* Check that lineno is incremented when we encounter a newline
	 * inside a quoted string.
	 */
	TEST_FEATURE ("with newline inside quoted string and lineno set");
	pos = 0;
	lineno = 1;
	len = 0;
	ret = nih_config_token (buf, strlen (buf), &pos, &lineno,
				NULL, " ", FALSE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 11);
	TEST_EQ (pos, 13);
	TEST_EQ (lineno, 2);


	/* Check that an escaped newline, and surrounding whitespace, is
	 * treated as a single space character.
	 */
	TEST_FEATURE ("with escaped newline");
	strcpy (buf, "this \\\n is a:test");
	pos = 0;
	lineno = 1;
	len = 0;
	ret = nih_config_token (buf, strlen (buf), &pos, &lineno,
				NULL, ":", FALSE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 9);
	TEST_EQ (pos, 12);
	TEST_EQ (lineno, 2);


	/* Check that extracting a token with an escaped newline inside it only
	 * returns a single space for the newline.
	 */
	TEST_FEATURE ("with escaped newline inside extracted string");
	ret = nih_config_token (buf, strlen (buf), NULL, NULL,
				dest, ":", FALSE, NULL);

	TEST_EQ (ret, 0);
	TEST_EQ_STR (dest, "this is a");


	/* Check that lineno is incremented when we encounter an escaped
	 * newline
	 */
	TEST_FEATURE ("with escaped newline inside string and lineno set");
	pos = 0;
	lineno = 1;
	len = 0;
	ret = nih_config_token (buf, strlen (buf), &pos, &lineno,
				NULL, ":", FALSE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 9);
	TEST_EQ (pos, 12);
	TEST_EQ (lineno, 2);


	/* Check that we can obtain the length of a token that contains
	 * escaped characters, the length should include the backslashes.
	 */
	TEST_FEATURE ("with escaped characters inside token");
	strcpy (buf, "this\\$FOO");
	pos = 0;
	len = 0;
	ret = nih_config_token (buf, strlen (buf), &pos, NULL,
				NULL, " ", FALSE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 9);
	TEST_EQ (pos, 9);


	/* Check that we can extract a token that contains escaped
	 * characters.
	 */
	TEST_FEATURE ("with escaped characters within extracted token");
	ret = nih_config_token (buf, strlen (buf), NULL, NULL,
				dest, " ", FALSE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 9);
	TEST_EQ_STR (dest, "this\\$FOO");


	/* Check that we can obtain the length of a token that contains
	 * escaped characters, including the backslashes, even though
	 * we're dequoting.
	 */
	TEST_FEATURE ("with escaped characters inside token and dequoting");
	pos = 0;
	len = 0;
	ret = nih_config_token (buf, strlen (buf), &pos, NULL,
				NULL, " ", TRUE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 9);
	TEST_EQ (pos, 9);


	/* Check that we can extract a token that contains escaped characters,
	 * which should include the backslashes even though we're dequoting.
	 */
	TEST_FEATURE ("with escaped characters within extracted dequoted token");
	len = 0;
	ret = nih_config_token (buf, strlen (buf), NULL, NULL,
				dest, " ", TRUE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 9);
	TEST_EQ_STR (dest, "this\\$FOO");


	/* Check that we can obtain the length of a token that contains
	 * escaped backslashes, the length should include the backslashes.
	 */
	TEST_FEATURE ("with escaped backslashes inside token");
	strcpy (buf, "this\\\\FOO");
	pos = 0;
	len = 0;
	ret = nih_config_token (buf, strlen (buf), &pos, NULL,
				NULL, " ", FALSE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 9);
	TEST_EQ (pos, 9);


	/* Check that we can extract a token that contains escaped
	 * blackslashes.
	 */
	TEST_FEATURE ("with escaped backslashes within extracted token");
	ret = nih_config_token (buf, strlen (buf), NULL, NULL,
				dest, " ", FALSE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 9);
	TEST_EQ_STR (dest, "this\\\\FOO");


	/* Check that we can obtain the length of a token that contains
	 * escaped blackslashes, reduced to one since we're dequoting
	 */
	TEST_FEATURE ("with escaped backslashes inside token and dequoting");
	pos = 0;
	len = 0;
	ret = nih_config_token (buf, strlen (buf), &pos, NULL,
				NULL, " ", TRUE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 8);
	TEST_EQ (pos, 9);


	/* Check that we can extract a token that contains escaped backslashes,
	 * which should include only one of the backslashes because
	 * we're dequoting.
	 */
	TEST_FEATURE ("with escaped backslashes within extracted dequoted token");
	len = 0;
	ret = nih_config_token (buf, strlen (buf), NULL, NULL,
				dest, " ", TRUE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 8);
	TEST_EQ_STR (dest, "this\\FOO");


	/* Check that a slash at the end of the file causes a parser error
	 * to be raised with pos and lineno set to the offending location.
	 */
	TEST_FEATURE ("with slash at end of string");
	strcpy (buf, "wibble\\");
	pos = 0;
	len = 0;
	lineno = 1;

	ret = nih_config_token (buf, strlen (buf), &pos, &lineno,
				NULL, " ", FALSE, NULL);

	TEST_LT (ret, 0);
	TEST_EQ (pos, 7);
	TEST_EQ (lineno, 1);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_CONFIG_TRAILING_SLASH);
	nih_free (err);


	/* Ceck that an unterminated quote causes a parser error to be
	 * raised, with pos and lineno set to the offending location.
	 */
	TEST_FEATURE ("with unterminated quote");
	strcpy (buf, "\"wibble\n");
	pos = 0;
	lineno = 1;

	ret = nih_config_token (buf, strlen (buf), &pos, &lineno,
				NULL, " ", FALSE, NULL);

	TEST_LT (ret, 0);
	TEST_EQ (pos, 8);
	TEST_EQ (lineno, 2);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_CONFIG_UNTERMINATED_QUOTE);
	nih_free (err);


	/* Check that an empty token results in the position left unchanged
	 * and zero being returned,
	 */
	TEST_FEATURE ("with empty token");
	strcpy (buf, " wibble");
	pos = 0;
	len = 0;
	ret = nih_config_token (buf, strlen (buf), &pos, NULL,
				NULL, " ", FALSE, &len);

	TEST_EQ (ret, 0);
	TEST_EQ (len, 0);
	TEST_EQ (pos, 0);
}

void
test_next_token (void)
{
	char      buf[1024];
	char     *str;
	size_t    pos, lineno;
	NihError *err;

	TEST_FUNCTION ("nih_config_next_token");

	/* Check that we can extract a token at the start of a string,
	 * and have the position pointing past the whitespace to the next
	 * argument.
	 */
	TEST_FEATURE ("with token at start of string");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is a test");
		pos = 0;

		str = nih_config_next_token (NULL, buf,
					     strlen (buf), &pos, NULL,
					     NIH_CONFIG_CNLWS, FALSE);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 5);
		TEST_ALLOC_SIZE (str, 5);
		TEST_EQ_STR (str, "this");

		nih_free (str);
	}


	/* Check that we can extract an argument inside a string
	 */
	TEST_FEATURE ("with token inside string");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is a test");
		pos = 5;

		str = nih_config_next_token (NULL, buf,
					     strlen (buf), &pos, NULL,
					     NIH_CONFIG_CNLWS, FALSE);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 5);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 8);
		TEST_ALLOC_SIZE (str, 3);
		TEST_EQ_STR (str, "is");

		nih_free (str);
	}


	/* Check that all trailing whitespace is eaten after the token. */
	TEST_FEATURE ("with consecutive whitespace after token");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this \t  is a test");
		pos = 0;

		str = nih_config_next_token (NULL, buf,
					     strlen (buf), &pos, NULL,
					     NIH_CONFIG_CNLWS, FALSE);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 8);
		TEST_ALLOC_SIZE (str, 5);
		TEST_EQ_STR (str, "this");

		nih_free (str);
	}


	/* Check that any escaped newlines in the whitespace are skipped
	 * over
	 */
	TEST_FEATURE ("with escaped newlines in whitespace");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this \\\n is a test");
		pos = 0;

		str = nih_config_next_token (NULL, buf,
					     strlen (buf), &pos, NULL,
					     NIH_CONFIG_CNLWS, FALSE);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 8);
		TEST_ALLOC_SIZE (str, 5);
		TEST_EQ_STR (str, "this");

		nih_free (str);
	}


	/* Check that the line number is incremented for any escaped newlines
	 * in the whitespace.
	 */
	TEST_FEATURE ("with line number set");
	TEST_ALLOC_FAIL {
		pos = 0;
		lineno = 1;

		str = nih_config_next_token (NULL, buf,
					     strlen (buf), &pos, &lineno,
					     NIH_CONFIG_CNLWS, FALSE);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);
			TEST_EQ (lineno, 2);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 8);
		TEST_EQ (lineno, 2);
		TEST_ALLOC_SIZE (str, 5);
		TEST_EQ_STR (str, "this");

		nih_free (str);
	}


	/* Check that the returned token can have the quotes left in it,
	 * but the whitespace around the newline collapsed.
	 */
	TEST_FEATURE ("with token containing quotes");
	TEST_ALLOC_FAIL {
		strcpy (buf, "\"this \\\n is\" a test");
		pos = 0;

		str = nih_config_next_token (NULL, buf,
					     strlen (buf), &pos, NULL,
					     NIH_CONFIG_CNLWS, FALSE);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 13);
		TEST_ALLOC_SIZE (str, 10);
		TEST_EQ_STR (str, "\"this is\"");

		nih_free (str);
	}


	/* Check that the returned token can be thoroughly dequoted and any
	 * whitespace around an embedded newline collapsed to a single
	 * space.
	 */
	TEST_FEATURE ("with quoted whitespace and newline in token");
	TEST_ALLOC_FAIL {
		strcpy (buf, "\"this \\\n is\" a test");
		pos = 0;

		str = nih_config_next_token (NULL, buf,
					     strlen (buf), &pos, NULL,
					     NIH_CONFIG_CNLWS, TRUE);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 13);
		TEST_ALLOC_SIZE (str, 8);
		TEST_EQ_STR (str, "this is");

		nih_free (str);
	}


	/* Check that an error is raised if there is no token at that
	 * position.
	 */
	TEST_FEATURE ("with empty line");
	TEST_ALLOC_FAIL {
		strcpy (buf, "\nthis is a test");
		pos = 0;
		lineno = 1;

		str = nih_config_next_token (NULL, buf,
					     strlen (buf), &pos, &lineno,
					     NIH_CONFIG_CNLWS, FALSE);

		TEST_EQ_P (str, NULL);
		TEST_EQ (pos, 0);
		TEST_EQ (lineno, 1);

		err = nih_error_get ();
		TEST_EQ (err->number, NIH_CONFIG_EXPECTED_TOKEN);
		nih_free (err);
	}


	/* Check that a parse error being found with the argument causes an
	 * error to be raised, with pos and lineno at the site of the error.
	 */
	TEST_FEATURE ("with parser error");
	TEST_ALLOC_FAIL {
		strcpy (buf, "\"this is a test\nand so is this");
		pos = 0;
		lineno = 1;

		str = nih_config_next_token (NULL, buf,
					     strlen (buf), &pos, &lineno,
					     NIH_CONFIG_CNLWS, FALSE);

		TEST_EQ_P (str, NULL);
		TEST_EQ (pos, 30);
		TEST_EQ (lineno, 2);

		err = nih_error_get ();
		TEST_EQ (err->number, NIH_CONFIG_UNTERMINATED_QUOTE);
		nih_free (err);
	}
}

void
test_next_arg (void)
{
	char      buf[1024];
	char     *str;
	size_t    pos, lineno;
	NihError *err;

	TEST_FUNCTION ("nih_config_next_arg");

	/* Check that we can extract an argument at the start of a string,
	 * and have the position pointing past the whitespace to the next
	 * argument.
	 */
	TEST_FEATURE ("with argument at start of string");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is a test");
		pos = 0;

		str = nih_config_next_arg (NULL, buf,
					   strlen (buf), &pos, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 5);
		TEST_ALLOC_SIZE (str, 5);
		TEST_EQ_STR (str, "this");

		nih_free (str);
	}


	/* Check that we can extract an argument inside a string
	 */
	TEST_FEATURE ("with argument inside string");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is a test");
		pos = 5;

		str = nih_config_next_arg (NULL, buf,
					   strlen (buf), &pos, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 5);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 8);
		TEST_ALLOC_SIZE (str, 3);
		TEST_EQ_STR (str, "is");

		nih_free (str);
	}


	/* Check that all trailing whitespace is eaten after the argument. */
	TEST_FEATURE ("with consecutive whitespace after argument");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this \t  is a test");
		pos = 0;

		str = nih_config_next_arg (NULL, buf,
					   strlen (buf), &pos, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 8);
		TEST_ALLOC_SIZE (str, 5);
		TEST_EQ_STR (str, "this");

		nih_free (str);
	}


	/* Check that any escaped newlines in the whitespace are skipped
	 * over
	 */
	TEST_FEATURE ("with escaped newlines in whitespace");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this \\\n is a test");
		pos = 0;

		str = nih_config_next_arg (NULL, buf,
					   strlen (buf), &pos, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 8);
		TEST_ALLOC_SIZE (str, 5);
		TEST_EQ_STR (str, "this");

		nih_free (str);
	}


	/* Check that the line number is incremented for any escaped newlines
	 * in the whitespace.
	 */
	TEST_FEATURE ("with line number set");
	TEST_ALLOC_FAIL {
		pos = 0;
		lineno = 1;

		str = nih_config_next_arg (NULL, buf,
					   strlen (buf), &pos, &lineno);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);
			TEST_EQ (lineno, 2);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 8);
		TEST_EQ (lineno, 2);
		TEST_ALLOC_SIZE (str, 5);
		TEST_EQ_STR (str, "this");

		nih_free (str);
	}


	/* Check that the returned argument is thoroughly dequoted and any
	 * whitespace around an embedded newline collapsed to a single
	 * space.
	 */
	TEST_FEATURE ("with quoted whitespace and newline in arg");
	TEST_ALLOC_FAIL {
		strcpy (buf, "\"this \\\n is\" a test");
		pos = 0;

		str = nih_config_next_arg (NULL, buf,
					   strlen (buf), &pos, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 13);
		TEST_ALLOC_SIZE (str, 8);
		TEST_EQ_STR (str, "this is");

		nih_free (str);
	}


	/* Check that an error is raised if there is no argument at that
	 * position.
	 */
	TEST_FEATURE ("with empty line");
	TEST_ALLOC_FAIL {
		strcpy (buf, "\nthis is a test");
		pos = 0;
		lineno = 1;

		str = nih_config_next_arg (NULL, buf,
					   strlen (buf), &pos, &lineno);

		TEST_EQ_P (str, NULL);
		TEST_EQ (pos, 0);
		TEST_EQ (lineno, 1);

		err = nih_error_get ();
		TEST_EQ (err->number, NIH_CONFIG_EXPECTED_TOKEN);
		nih_free (err);
	}


	/* Check that a parse error being found with the argument causes an
	 * error to be raised, with pos and lineno at the site of the error.
	 */
	TEST_FEATURE ("with parser error");
	TEST_ALLOC_FAIL {
		strcpy (buf, "\"this is a test\nand so is this");
		pos = 0;
		lineno = 1;

		str = nih_config_next_arg (NULL, buf,
					   strlen (buf), &pos, &lineno);

		TEST_EQ_P (str, NULL);
		TEST_EQ (pos, 30);
		TEST_EQ (lineno, 2);

		err = nih_error_get ();
		TEST_EQ (err->number, NIH_CONFIG_UNTERMINATED_QUOTE);
		nih_free (err);
	}
}

void
test_next_line (void)
{
	char   buf[1024];
	size_t pos, lineno;

	TEST_FUNCTION ("nih_config_next_line");

	/* Check that we can skip a number of characters until the newline,
	 * pointing pos past it.
	 */
	TEST_FEATURE ("with simple string");
	strcpy (buf, "this is a test\nand so is this\n");
	pos = 0;

	nih_config_next_line (buf, strlen (buf), &pos, NULL);

	TEST_EQ (pos, 15);


	/* Check that lineno is incremented when we step over it.
	 */
	TEST_FEATURE ("with line number set");
	pos = 0;
	lineno = 1;

	nih_config_next_line (buf, strlen (buf), &pos, &lineno);

	TEST_EQ (pos, 15);
	TEST_EQ (lineno, 2);


	/* Check that pos is only incremented by a single step if the
	 * character underneath is a newline.
	 */
	TEST_FEATURE ("with newline at position");
	strcpy (buf, "\nthis is a test");
	pos = 0;
	lineno = 1;

	nih_config_next_line (buf, strlen (buf), &pos, &lineno);

	TEST_EQ (pos, 1);
	TEST_EQ (lineno, 2);


	/* Check that the end of file can be reached without error.
	 */
	TEST_FEATURE ("with no newline before end of file");
	strcpy (buf, "this is a test");
	pos = 0;

	nih_config_next_line (buf, strlen (buf), &pos, NULL);

	TEST_EQ (pos, 14);
}

void
test_skip_whitespace (void)
{
	char   buf[1024];
	size_t pos, lineno;

	TEST_FUNCTION ("nih_config_next_whitespace");

	/* Check that we can skip an amount of plain whitespace characters
	 * until the next token, pointing pos at is.
	 */
	TEST_FEATURE ("with plain whitespace");
	strcpy (buf, "a  plain string\n");
	pos = 1;
	lineno = 1;

	nih_config_skip_whitespace (buf, strlen (buf), &pos, &lineno);

	TEST_EQ (pos, 3);
	TEST_EQ (lineno, 1);


	/* Check that we can skip a more complex series of whitespace
	 * characters until the next token.
	 */
	TEST_FEATURE ("with complex whitespace");
	strcpy (buf, "a more   \t  \r  complex string\n");
	pos = 6;
	lineno = 1;

	nih_config_skip_whitespace (buf, strlen (buf), &pos, &lineno);

	TEST_EQ (pos, 15);
	TEST_EQ (lineno, 1);


	/* Check that we can skip whitespace characters up until the end
	 * of the line, but that we don't step over it.
	 */
	TEST_FEATURE ("with whitespace at end of line");
	strcpy (buf, "trailing whitespace  \t\r\n");
	pos = 19;
	lineno = 1;

	nih_config_skip_whitespace (buf, strlen (buf), &pos, &lineno);

	TEST_EQ (pos, 23);
	TEST_EQ (lineno, 1);


	/* Check that we step over an escaped newline embedded in the
	 * whitespace, and increment lineno.
	 */
	TEST_FEATURE ("with escaped newline");
	strcpy (buf, "this has \\\n a newline");
	pos = 8;
	lineno = 1;

	nih_config_skip_whitespace (buf, strlen (buf), &pos, &lineno);

	TEST_EQ (pos, 12);
	TEST_EQ (lineno, 2);
}

void
test_skip_comment (void)
{
	char      buf[1024];
	size_t    pos, lineno;
	int       ret;
	NihError *err;

	TEST_FUNCTION ("nih_config_next_line");

	/* Check that we can skip a number of comment characters until the
	 * newline,  pointing pos past it.
	 */
	TEST_FEATURE ("with simple string");
	strcpy (buf, "# this is a test\nand so is this\n");
	pos = 0;

	ret = nih_config_skip_comment (buf, strlen (buf), &pos, NULL);

	TEST_EQ (ret, 0);
	TEST_EQ (pos, 17);


	/* Check that lineno is incremented when we step over it.
	 */
	TEST_FEATURE ("with line number set");
	pos = 0;
	lineno = 1;

	ret = nih_config_skip_comment (buf, strlen (buf), &pos, &lineno);

	TEST_EQ (ret, 0);
	TEST_EQ (pos, 17);
	TEST_EQ (lineno, 2);


	/* Check that pos is only incremented by a single step if the
	 * character underneath is a newline.
	 */
	TEST_FEATURE ("with newline at position");
	strcpy (buf, "\nthis is a test");
	pos = 0;
	lineno = 1;

	ret = nih_config_skip_comment (buf, strlen (buf), &pos, &lineno);

	TEST_EQ (ret, 0);
	TEST_EQ (pos, 1);
	TEST_EQ (lineno, 2);


	/* Check that the end of file can be reached without error.
	 */
	TEST_FEATURE ("with no newline before end of file");
	strcpy (buf, "# this is a test");
	pos = 0;

	ret = nih_config_skip_comment (buf, strlen (buf), &pos, NULL);

	TEST_EQ (ret, 0);
	TEST_EQ (pos, 16);


	/* Check that attempting to skip an ordinary argument results in
	 * an error.
	 */
	TEST_FEATURE ("with attempt to skip argument");
	strcpy (buf, "this is a test\nand so it this\n");
	pos = 0;

	ret = nih_config_skip_comment (buf, strlen (buf), &pos, NULL);

	TEST_LT (ret, 0);
	TEST_EQ (pos, 0);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_CONFIG_UNEXPECTED_TOKEN);
	nih_free (err);
}


void
test_parse_args (void)
{
	char       buf[1024];
	char     **args;
	size_t     pos, lineno;
	NihError  *err;

	TEST_FUNCTION ("nih_config_parse_args");

	/* Check that we can parse a list of arguments from the start of
	 * a simple string.  They should be returned as a NULL-terminated
	 * array of strings, and the position should be updated to point to
	 * the start of the next line.
	 */
	TEST_FEATURE ("with args at start of simple string");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is a test\nand so is this\n");
		pos = 0;

		args = nih_config_parse_args (NULL, buf,
					      strlen (buf), &pos, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (args, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 15);
		TEST_ALLOC_SIZE (args, sizeof (char *) * 5);
		TEST_ALLOC_PARENT (args[0], args);
		TEST_ALLOC_PARENT (args[1], args);
		TEST_ALLOC_PARENT (args[2], args);
		TEST_ALLOC_PARENT (args[3], args);
		TEST_EQ_STR (args[0], "this");
		TEST_EQ_STR (args[1], "is");
		TEST_EQ_STR (args[2], "a");
		TEST_EQ_STR (args[3], "test");
		TEST_EQ_P (args[4], NULL);

		nih_free (args);
	}


	/* Check that we can parse a list of arguments from a position
	 * inside an existing string.
	 */
	TEST_FEATURE ("with args inside simple string");
	TEST_ALLOC_FAIL {
		pos = 5;

		args = nih_config_parse_args (NULL, buf,
					      strlen (buf), &pos, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (args, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 15);
		TEST_ALLOC_SIZE (args, sizeof (char *) * 4);
		TEST_EQ_STR (args[0], "is");
		TEST_EQ_STR (args[1], "a");
		TEST_EQ_STR (args[2], "test");
		TEST_EQ_P (args[3], NULL);

		nih_free (args);
	}


	/* Check that we can parse a list of arguments up to the end of the
	 * file, which doesn't have a newline.
	 */
	TEST_FEATURE ("with args up to end of string");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is a test");
		pos = 0;

		args = nih_config_parse_args (NULL, buf,
					      strlen (buf), &pos, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (args, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 14);
		TEST_ALLOC_SIZE (args, sizeof (char *) * 5);
		TEST_EQ_STR (args[0], "this");
		TEST_EQ_STR (args[1], "is");
		TEST_EQ_STR (args[2], "a");
		TEST_EQ_STR (args[3], "test");
		TEST_EQ_P (args[4], NULL);

		nih_free (args);
	}


	/* Check that we can ignore a comment at the end of the line, the
	 * position should be updated past the comment onto the next line.
	 */
	TEST_FEATURE ("with args up to comment");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is a test # comment\nand so is this\n");
		pos = 0;

		args = nih_config_parse_args (NULL, buf,
					      strlen (buf), &pos, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (args, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 25);
		TEST_ALLOC_SIZE (args, sizeof (char *) * 5);
		TEST_EQ_STR (args[0], "this");
		TEST_EQ_STR (args[1], "is");
		TEST_EQ_STR (args[2], "a");
		TEST_EQ_STR (args[3], "test");
		TEST_EQ_P (args[4], NULL);

		nih_free (args);
	}


	/* Check that we can ignore a comment at the end of the file, the
	 * position should be updated past the end.
	 */
	TEST_FEATURE ("with args up to comment at end of file");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is a test # comment");
		pos = 0;

		args = nih_config_parse_args (NULL, buf,
					      strlen (buf), &pos, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (args, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 24);
		TEST_ALLOC_SIZE (args, sizeof (char *) * 5);
		TEST_EQ_STR (args[0], "this");
		TEST_EQ_STR (args[1], "is");
		TEST_EQ_STR (args[2], "a");
		TEST_EQ_STR (args[3], "test");
		TEST_EQ_P (args[4], NULL);

		nih_free (args);
	}


	/* Check that the line number is incremented when a new line is
	 * encountered.
	 */
	TEST_FEATURE ("with line number given");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is a test\nand so is this\n");
		pos = 0;
		lineno = 1;

		args = nih_config_parse_args (NULL, buf,
					      strlen (buf), &pos, &lineno);

		if (test_alloc_failed) {
			TEST_EQ_P (args, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 15);
		TEST_EQ (lineno, 2);

		nih_free (args);
	}


	/* Check that consecutive whitespace, including escaped newlines,
	 * are treated as a single delimeter.  The line number should be
	 * incremented for both the embedded one and final one.
	 */
	TEST_FEATURE ("with multiple whitespace between arguments");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this   is \t  a  \\\n test\nand so is this\n");
		pos = 0;
		lineno = 1;

		args = nih_config_parse_args (NULL, buf,
					      strlen (buf), &pos, &lineno);

		if (test_alloc_failed) {
			TEST_EQ_P (args, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 24);
		TEST_EQ (lineno, 3);
		TEST_ALLOC_SIZE (args, sizeof (char *) * 5);
		TEST_EQ_STR (args[0], "this");
		TEST_EQ_STR (args[1], "is");
		TEST_EQ_STR (args[2], "a");
		TEST_EQ_STR (args[3], "test");
		TEST_EQ_P (args[4], NULL);

		nih_free (args);
	}


	/* Check that each argument can be delimited by quotes, contain
	 * quoted newlines, and each is dequoted before being stored in the
	 * args array,
	 */
	TEST_FEATURE ("with whitespace inside arguments");
	TEST_ALLOC_FAIL {
		strcpy (buf, "\"this is\" \"a\ntest\" \\\n and so\nis this\n");
		pos = 0;
		lineno = 1;

		args = nih_config_parse_args (NULL, buf,
					      strlen (buf), &pos, &lineno);

		if (test_alloc_failed) {
			TEST_EQ_P (args, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 29);
		TEST_EQ (lineno, 4);
		TEST_ALLOC_SIZE (args, sizeof (char *) * 5);
		TEST_EQ_STR (args[0], "this is");
		TEST_EQ_STR (args[1], "a test");
		TEST_EQ_STR (args[2], "and");
		TEST_EQ_STR (args[3], "so");
		TEST_EQ_P (args[4], NULL);

		nih_free (args);
	}


	/* Check that an empty line results in a one element array being
	 * returned containing only NULL, and the position being incremented
	 * past the empty line.
	 */
	TEST_FEATURE ("with empty line");
	TEST_ALLOC_FAIL {
		strcpy (buf, "\nand so is this\n");
		pos = 0;

		args = nih_config_parse_args (NULL, buf,
					      strlen (buf), &pos, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (args, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 1);
		TEST_ALLOC_SIZE (args, sizeof (char *) * 1);
		TEST_EQ_P (args[0], NULL);

		nih_free (args);
	}


	/* Check that a line containing only a comment results in a one
	 * element array being returned containing only NULL, and the
	 * position being incremented past the comment and newline.
	 */
	TEST_FEATURE ("with only comment in line");
	TEST_ALLOC_FAIL {
		strcpy (buf, "# line with comment\nand so is this\n");
		pos = 0;

		args = nih_config_parse_args (NULL, buf,
					      strlen (buf), &pos, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (args, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 20);
		TEST_ALLOC_SIZE (args, sizeof (char *) * 1);
		TEST_EQ_P (args[0], NULL);

		nih_free (args);
	}


	/* Check that an error parsing the arguments results in NULL being
	 * returned and the error raised.
	 */
	TEST_FEATURE ("with parser error");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is a \"test\nand so is this\n");
		pos = 0;
		lineno = 1;

		args = nih_config_parse_args (NULL, buf,
					      strlen (buf), &pos, &lineno);

		TEST_EQ_P (args, NULL);
		if (! test_alloc_failed) {
			TEST_EQ (pos, 31);
			TEST_EQ (lineno, 3);
		}

		err = nih_error_get ();
		if (! test_alloc_failed)
			TEST_EQ (err->number, NIH_CONFIG_UNTERMINATED_QUOTE);
		nih_free (err);
	}
}

void
test_parse_command (void)
{
	char       buf[1024];
	char      *str;
	size_t     pos, lineno;
	NihError  *err;

	TEST_FUNCTION ("nih_config_parse_command");

	/* Check that we can parse a command from the start of a simple
	 * string.  It should be returned as an allocated string and the
	 * position should be updated to point to the start of the next line.
	 */
	TEST_FEATURE ("with command at start of simple string");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is a test\nand so is this\n");
		pos = 0;

		str = nih_config_parse_command (NULL, buf,
						strlen (buf), &pos, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 15);
		TEST_ALLOC_SIZE (str, 15);
		TEST_EQ_STR (str, "this is a test");

		nih_free (str);
	}


	/* Check that we can parse a command from inside a string.
	 */
	TEST_FEATURE ("with command inside simple string");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is a test\nand so is this\n");
		pos = 5;

		str = nih_config_parse_command (NULL, buf,
						strlen (buf), &pos, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 5);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 15);
		TEST_ALLOC_SIZE (str, 10);
		TEST_EQ_STR (str, "is a test");

		nih_free (str);
	}


	/* Check that we can parse a command that ends with the end of file.
	 */
	TEST_FEATURE ("with command at end of file");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is a test");
		pos = 0;

		str = nih_config_parse_command (NULL, buf,
						strlen (buf), &pos, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 14);
		TEST_ALLOC_SIZE (str, 15);
		TEST_EQ_STR (str, "this is a test");

		nih_free (str);
	}


	/* Check that we can parse a command that ends with a comment,
	 * but the position should be incremented past the end of the comment.
	 */
	TEST_FEATURE ("with command up to comment");
	TEST_ALLOC_FAIL {
		strcpy (buf, ("this is a test # this is a comment\n"
			      "and so is this\n"));
		pos = 0;
		lineno = 1;

		str = nih_config_parse_command (NULL, buf, strlen (buf), &pos,
						&lineno);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);
			TEST_EQ (lineno, 2);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 35);
		TEST_EQ (lineno, 2);
		TEST_ALLOC_SIZE (str, 15);
		TEST_EQ_STR (str, "this is a test");

		nih_free (str);
	}


	/* Check that we can parse a command that ends with a comment which
	 * runs up to the end of file.
	 */
	TEST_FEATURE ("with command up to comment at end of file");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is a test # this is a comment");
		pos = 0;
		lineno = 1;

		str = nih_config_parse_command (NULL, buf, strlen (buf), &pos,
						&lineno);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);
			TEST_EQ (lineno, 1);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 34);
		TEST_EQ (lineno, 1);
		TEST_ALLOC_SIZE (str, 15);
		TEST_EQ_STR (str, "this is a test");

		nih_free (str);
	}


	/* Check that the command is returned including any quotes,
	 * consecutive whitespace, but with any whitespace around a quoted
	 * or escaped newline collapsed to a single space.
	 */
	TEST_FEATURE ("with quotes, whitespace and newlines in string");
	TEST_ALLOC_FAIL {
		strcpy (buf, ("\"this   is\" a \"test \\\n of\" \\\n "
			      "commands\nfoo\n"));
		pos = 0;
		lineno = 1;

		str = nih_config_parse_command (NULL, buf, strlen (buf), &pos,
						&lineno);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 39);
		TEST_EQ (lineno, 4);
		TEST_ALLOC_SIZE (str, 33);
		TEST_EQ_STR (str, "\"this   is\" a \"test of\" commands");

		nih_free (str);
	}


	/* Check that we can parse an empty line, and have the empty string
	 * returned.  The position should be updated past the newline.
	 */
	TEST_FEATURE ("with empty line");
	TEST_ALLOC_FAIL {
		strcpy (buf, "\nthis is a test\n");
		pos = 0;

		str = nih_config_parse_command (NULL, buf,
						strlen (buf), &pos, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 1);
		TEST_ALLOC_SIZE (str, 1);
		TEST_EQ_STR (str, "");

		nih_free (str);
	}


	/* Check that we can parse a line containing only whitespace, and
	 * have the empty string returned.  The position should be updated
	 * past the newline.
	 */
	TEST_FEATURE ("with only whitespace in line");
	TEST_ALLOC_FAIL {
		strcpy (buf, "  \t  \nthis is a test\n");
		pos = 0;

		str = nih_config_parse_command (NULL, buf,
						strlen (buf), &pos, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 6);
		TEST_ALLOC_SIZE (str, 1);
		TEST_EQ_STR (str, "");

		nih_free (str);
	}


	/* Check that we can parse a line with a comment in it, and have
	 * the empty string returned.  The position should be updated past
	 * the newline.
	 */
	TEST_FEATURE ("with only comment in line");
	TEST_ALLOC_FAIL {
		strcpy (buf, "# this is a test\nthis is a test\n");
		pos = 0;

		str = nih_config_parse_command (NULL, buf,
						strlen (buf), &pos, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 17);
		TEST_ALLOC_SIZE (str, 1);
		TEST_EQ_STR (str, "");

		nih_free (str);
	}


	/* Check that we can parse a line with whitespace before a comment,
	 * and have the empty string returned.  The position should be updated
	 * past the newline.
	 */
	TEST_FEATURE ("with whitespace and comment in line");
	TEST_ALLOC_FAIL {
		strcpy (buf, "  # this is a test\nthis is a test\n");
		pos = 0;

		str = nih_config_parse_command (NULL, buf,
						strlen (buf), &pos, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 19);
		TEST_ALLOC_SIZE (str, 1);
		TEST_EQ_STR (str, "");

		nih_free (str);
	}


	/* Check that a parser error while reading the command results in
	 * NULL being returned and the error raised.
	 */
	TEST_FEATURE ("with parser error");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is a \"test\nand so is this\n");
		pos = 0;
		lineno = 1;

		str = nih_config_parse_command (NULL, buf, strlen (buf), &pos,
						&lineno);

		TEST_EQ_P (str, NULL);
		TEST_EQ (pos, 31);
		TEST_EQ (lineno, 3);

		err = nih_error_get ();
		TEST_EQ (err->number, NIH_CONFIG_UNTERMINATED_QUOTE);
		nih_free (err);
	}
}


void
test_parse_block (void)
{
	char      buf[1024];
	char     *str;
	size_t    pos, lineno;
	NihError *err;

	TEST_FUNCTION ("nih_config_parse_block");
	program_name = "test";


	/* Check that we can parse consecutive lines until we reach one
	 * that ends the block.  The block should be returned as an allocated
	 * string with each line in it, except the terminator; the position
	 * should be positioned after the end of the terminator.
	 */
	TEST_FEATURE ("with simple block");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is\na test\nend foo\nblah\n");
		pos = 0;

		str = nih_config_parse_block (NULL, buf,
					      strlen (buf), &pos, NULL, "foo");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 23);
		TEST_ALLOC_SIZE (str, 16);
		TEST_EQ_STR (str, "this is\na test\n");

		nih_free (str);
	}


	/* Check that the line number is incremented for each line that we
	 * discover in the block, including the terminating line.
	 */
	TEST_FEATURE ("with line number set");
	TEST_ALLOC_FAIL {
		pos = 0;
		lineno = 2;

		str = nih_config_parse_block (NULL, buf,
					      strlen (buf), &pos, &lineno,
					      "foo");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 23);
		TEST_EQ (lineno, 5);
		TEST_ALLOC_SIZE (str, 16);
		TEST_EQ_STR (str, "this is\na test\n");

		nih_free (str);
	}


	/* Check that the common initial whitespace from each line is stripped,
	 * where common is defined as identical character sequences, not number
	 * of whitespace chars.
	 */
	TEST_FEATURE ("with whitespace at start of block");
	TEST_ALLOC_FAIL {
		strcpy (buf, "    this is\n  \t a test\nend foo\nblah\n");
		pos = 0;

		str = nih_config_parse_block (NULL, buf,
					      strlen (buf), &pos, NULL, "foo");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 31);
		TEST_ALLOC_SIZE (str, 20);
		TEST_EQ_STR (str, "  this is\n\t a test\n");

		nih_free (str);
	}


	/* Check that we can parse a block that ends in a terminator with
	 * extraneous whitespace around the words.
	 */
	TEST_FEATURE ("with whitespace in terminator");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is\na test\n  end \t foo  \nblah\n");
		pos = 0;

		str = nih_config_parse_block (NULL, buf,
					      strlen (buf), &pos, NULL, "foo");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 29);
		TEST_ALLOC_SIZE (str, 16);
		TEST_EQ_STR (str, "this is\na test\n");

		nih_free (str);
	}


	/* Check that we can parse a block that ends in a terminator which
	 * is at the end of the file.
	 */
	TEST_FEATURE ("with terminator at end of file");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is\na test\nend foo");
		pos = 0;

		str = nih_config_parse_block (NULL, buf,
					      strlen (buf), &pos, NULL, "foo");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 22);
		TEST_ALLOC_SIZE (str, 16);
		TEST_EQ_STR (str, "this is\na test\n");

		nih_free (str);
	}


	/* Check that we can parse a block that ends in a terminator which
	 * has a comment following it.
	 */
	TEST_FEATURE ("with terminator and comment");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is\na test\nend foo # comment\ntest\n");
		pos = 0;

		str = nih_config_parse_block (NULL, buf,
					      strlen (buf), &pos, NULL, "foo");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 33);
		TEST_ALLOC_SIZE (str, 16);
		TEST_EQ_STR (str, "this is\na test\n");

		nih_free (str);
	}


	/* Check that we can parse a block that ends in a terminator which
	 * has a comment and then the end of file.
	 */
	TEST_FEATURE ("with terminator and comment at end of file");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is\na test\nend foo # comment");
		pos = 0;

		str = nih_config_parse_block (NULL, buf,
					      strlen (buf), &pos, NULL, "foo");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 32);
		TEST_ALLOC_SIZE (str, 16);
		TEST_EQ_STR (str, "this is\na test\n");

		nih_free (str);
	}


	/* Check that various bogus forms of terminator are ignored.
	 */
	TEST_FEATURE ("with various things that aren't terminators");
	TEST_ALLOC_FAIL {
		strcpy (buf, "endfoo\nend a\nend fooish\nend foo\ntest\n");
		pos = 0;

		str = nih_config_parse_block (NULL, buf,
					      strlen (buf), &pos, NULL, "foo");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_EQ (pos, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (pos, 32);
		TEST_ALLOC_SIZE (str, 25);
		TEST_EQ_STR (str, "endfoo\nend a\nend fooish\n");

		nih_free (str);
	}


	/* Check that reaching the end of the file without finding the block
	 * terminator causes an error to be raised and NULL to be returned.
	 */
	TEST_FEATURE ("with no terminator before end of file");
	TEST_ALLOC_FAIL {
		strcpy (buf, "this is\na test\n");
		pos = 0;
		lineno = 2;

		str = nih_config_parse_block (NULL, buf,
					      strlen (buf), &pos, &lineno,
					      "foo");

		TEST_EQ_P (str, NULL);
		TEST_EQ (pos, 15);
		TEST_EQ (lineno, 4);

		err = nih_error_get ();
		TEST_EQ (err->number, NIH_CONFIG_UNTERMINATED_BLOCK);
		nih_free (err);
	}
}

void
test_skip_block (void)
{
	char      buf[1024];
	int       ret;
	size_t    pos, lineno, endpos;
	NihError *err;

	TEST_FUNCTION ("nih_config_skip_block");
	program_name = "test";


	/* Check that we can find the end of a simple block.  pos should be
	 * updated to point past the block, and the returned endpos should
	 * point at the end of the block itself.
	 */
	TEST_FEATURE ("with simple block");
	strcpy (buf, "this is\na test\nend foo\nblah\n");
	pos = 0;

	ret = nih_config_skip_block (buf, strlen (buf), &pos, NULL,
				     "foo", &endpos);

	TEST_EQ (ret, 0);
	TEST_EQ (pos, 23);
	TEST_EQ (endpos, 15);


	/* Check that the line number is incremented for each line that we
	 * discover in the block, including the terminating line.
	 */
	TEST_FEATURE ("with line number set");
	pos = 0;
	lineno = 2;

	ret = nih_config_skip_block (buf, strlen (buf), &pos, &lineno,
				     "foo", &endpos);

	TEST_EQ (ret, 0);
	TEST_EQ (pos, 23);
	TEST_EQ (endpos, 15);
	TEST_EQ (lineno, 5);


	/* Check that we can find the end of a block that ends in a terminator
	 * with extraneous whitespace around the words.
	 */
	TEST_FEATURE ("with whitespace in terminator");
	strcpy (buf, "this is\na test\n  end \t foo  \nblah\n");
	pos = 0;

	ret = nih_config_skip_block (buf, strlen (buf), &pos, NULL,
				     "foo", &endpos);

	TEST_EQ (ret, 0);
	TEST_EQ (pos, 29);
	TEST_EQ (endpos, 15);


	/* Check that we can find the end of a block that ends in a
	 * terminator which is at the end of the file.
	 */
	TEST_FEATURE ("with terminator at end of file");
	strcpy (buf, "this is\na test\nend foo");
	pos = 0;

	ret = nih_config_skip_block (buf, strlen (buf), &pos, NULL,
				     "foo", &endpos);

	TEST_EQ (ret, 0);
	TEST_EQ (pos, 22);
	TEST_EQ (endpos, 15);


	/* Check that we can find the end of a block that ends in a
	 * terminator which has a comment following it.
	 */
	TEST_FEATURE ("with terminator and comment");
	strcpy (buf, "this is\na test\nend foo # comment\ntest\n");
	pos = 0;

	ret = nih_config_skip_block (buf, strlen (buf), &pos, NULL,
				     "foo", &endpos);

	TEST_EQ (ret, 0);
	TEST_EQ (pos, 33);
	TEST_EQ (endpos, 15);


	/* Check that we can find the end of a block that ends in a
	 * terminator which has a comment and then the end of file.
	 */
	TEST_FEATURE ("with terminator and comment at end of file");
	strcpy (buf, "this is\na test\nend foo # comment");
	pos = 0;

	ret = nih_config_skip_block (buf, strlen (buf), &pos, NULL,
				     "foo", &endpos);

	TEST_EQ (ret, 0);
	TEST_EQ (pos, 32);
	TEST_EQ (endpos, 15);


	/* Check that various bogus forms of terminator are ignored.
	 */
	TEST_FEATURE ("with various things that aren't terminators");
	strcpy (buf, "endfoo\nend a\nend fooish\nend foo\ntest\n");
	pos = 0;

	ret = nih_config_skip_block (buf, strlen (buf), &pos, NULL,
				     "foo", &endpos);

	TEST_EQ (ret, 0);
	TEST_EQ (pos, 32);
	TEST_EQ (endpos, 24);


	/* Check that reaching the end of the file without finding the block
	 * terminator causes an error to be raised and NULL to be returned.
	 */
	TEST_FEATURE ("with no terminator before end of file");
	strcpy (buf, "this is\na test\n");
	pos = 0;
	lineno = 2;

	ret = nih_config_skip_block (buf, strlen (buf), &pos, &lineno,
				     "foo", &endpos);

	TEST_LT (ret, 0);
	TEST_EQ (pos, 15);
	TEST_EQ (lineno, 4);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_CONFIG_UNTERMINATED_BLOCK);
	nih_free (err);
}


static int handler_called = 0;
static void *last_data = NULL;
static NihConfigStanza *last_stanza = NULL;
static const char *last_file = NULL;
static size_t last_len = 0;
static size_t last_pos = 0;
static size_t last_lineno = 0;

static int
my_handler (void            *data,
	    NihConfigStanza *stanza,
	    const char      *file,
	    size_t           len,
	    size_t          *pos,
	    size_t          *lineno)
{
	handler_called++;

	last_data = data;
	last_stanza = stanza;
	last_file = file;
	last_len = len;
	last_pos = *pos;
	if (lineno) {
		last_lineno = *lineno;
	} else {
		last_lineno = -1;
	}

	if (strcmp (stanza->name, "foo"))
		nih_config_next_line (file, len, pos, lineno);

	return 100;
}

static NihConfigStanza stanzas[] = {
	{ "foo", my_handler },
	{ "bar", my_handler },

	{ "frodo", my_handler },
	{ "bilbo", my_handler },

	NIH_CONFIG_LAST
};

static NihConfigStanza any_stanzas[] = {
	{ "", my_handler },

	NIH_CONFIG_LAST
};

void
test_parse_stanza (void)
{
	char      buf[1024];
	size_t    pos, lineno;
	int       ret;
	NihError *err;

	TEST_FUNCTION ("nih_config_stanza");
	program_name = "test";


	/* Check that the handler is called with all of the right arguments
	 * if the stanza is found at the start of the string.  The pos should
	 * only be incremented up to the point after the first argument,
	 * leaving it up to the stanza handler to increment it.
	 */
	TEST_FEATURE ("with stanza at start of string");
	strcpy (buf, "foo this is a test\nwibble\n");

	handler_called = 0;
	last_data = NULL;
	last_file = NULL;
	last_len = 0;
	last_pos = -1;
	last_lineno = 0;

	ret = nih_config_parse_stanza (buf, strlen (buf), NULL, NULL,
				       stanzas, &ret);

	TEST_TRUE (handler_called);
	TEST_EQ_P (last_data, &ret);
	TEST_EQ_P (last_file, buf);
	TEST_EQ (last_len, strlen (buf));
	TEST_EQ (last_pos, 4);
	TEST_EQ (last_lineno, (size_t)-1);

	TEST_EQ (ret, 100);


	/* Check that the handler can be called with a position inside the
	 * string.
	 */
	TEST_FEATURE ("with stanza inside string");
	strcpy (buf, "snarf foo this is a test\nwibble\n");
	pos = 6;

	handler_called = 0;
	last_data = NULL;
	last_file = NULL;
	last_len = 0;
	last_pos = -1;
	last_lineno = 0;

	ret = nih_config_parse_stanza (buf, strlen (buf), &pos, NULL,
				       stanzas, &ret);

	TEST_TRUE (handler_called);
	TEST_EQ_P (last_data, &ret);
	TEST_EQ_P (last_file, buf);
	TEST_EQ (last_len, strlen (buf));
	TEST_EQ (last_pos, 10);
	TEST_EQ (last_lineno, (size_t)-1);

	TEST_EQ (ret, 100);
	TEST_EQ (pos, 10);


	/* Check that the position can be updated by the handler function
	 * to point wherever it thinks the stanza ends.
	 */
	TEST_FEATURE ("with position moved by stanza");
	strcpy (buf, "bar this is a test\nwibble\n");
	pos = 0;
	lineno = 1;

	handler_called = 0;
	last_data = NULL;
	last_file = NULL;
	last_len = 0;
	last_pos = -1;
	last_lineno = 0;

	ret = nih_config_parse_stanza (buf, strlen (buf), &pos, &lineno,
				       stanzas, &ret);

	TEST_TRUE (handler_called);
	TEST_EQ_P (last_data, &ret);
	TEST_EQ_P (last_file, buf);
	TEST_EQ (last_len, strlen (buf));
	TEST_EQ (last_pos, 4);
	TEST_EQ (last_lineno, 1);

	TEST_EQ (ret, 100);
	TEST_EQ (pos, 19);
	TEST_EQ (lineno, 2);


	/* Check that finding an unknown stanza results in an error being
	 * raised, and no handler called.
	 */
	TEST_FEATURE ("with unknown stanza");
	strcpy (buf, "wibble this is a test\nwibble\n");
	pos = 0;
	lineno = 1;

	handler_called = 0;

	ret = nih_config_parse_stanza (buf, strlen (buf), &pos, &lineno,
				       stanzas, &ret);

	TEST_FALSE (handler_called);
	TEST_LT (ret, 0);
	TEST_EQ (pos, 0);
	TEST_EQ (lineno, 1);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_CONFIG_UNKNOWN_STANZA);
	nih_free (err);


	/* Check that unknown stanzas can be handled by an entry in the
	 * table with a zero-length name.
	 */
	TEST_FEATURE ("with unknown stanza and catch-all");
	pos = 0;

	handler_called = 0;
	last_data = NULL;
	last_file = NULL;
	last_len = 0;
	last_pos = -1;
	last_lineno = 0;

	ret = nih_config_parse_stanza (buf, strlen (buf), NULL, NULL,
				       any_stanzas, &ret);

	TEST_TRUE (handler_called);
	TEST_EQ_P (last_data, &ret);
	TEST_EQ_P (last_file, buf);
	TEST_EQ (last_len, strlen (buf));
	TEST_EQ (last_pos, 7);
	TEST_EQ (last_lineno, (size_t)-1);

	TEST_EQ (ret, 100);


	/* Check that an error is raised if there is no stanza at this
	 * position in the file.
	 */
	TEST_FEATURE ("with empty line");
	strcpy (buf, "\nfoo this is a test\n");
	pos = 0;
	lineno = 1;

	handler_called = 0;

	ret = nih_config_parse_stanza (buf, strlen (buf), &pos, &lineno,
				       stanzas, &ret);

	TEST_FALSE (handler_called);
	TEST_LT (ret, 0);
	TEST_EQ (pos, 0);
	TEST_EQ (lineno, 1);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_CONFIG_EXPECTED_TOKEN);
	nih_free (err);
}


void
test_parse_file (void)
{
	char      buf[1024];
	size_t    pos, lineno;
	int       ret;
	NihError *err;

	TEST_FUNCTION ("nih_config_parse_file");


	/* Check that a simple sequence of stanzas is parsed, with the
	 * handler being called for each.  When finished, the position
	 * should be past the end of the file.
	 */
	TEST_FEATURE ("with simple lines");
	strcpy (buf, "frodo test\nbilbo test\n");
	pos = 0;
	lineno = 1;

	handler_called = 0;
	last_data = NULL;
	last_file = NULL;
	last_len = 0;
	last_pos = -1;
	last_lineno = 0;

	ret = nih_config_parse_file (buf, strlen (buf), &pos, &lineno,
				     stanzas, &buf);

	TEST_EQ (ret, 0);
	TEST_EQ (pos, 22);

	TEST_EQ (handler_called, 2);
	TEST_EQ_P (last_data, &buf);
	TEST_EQ_P (last_file, buf);
	TEST_EQ (last_len, strlen (buf));
	TEST_EQ (last_pos, 17);
	TEST_EQ (last_lineno, 2);


	/* Check that a line ending in a comment can be parsed, with the
	 * comment skipped.
	 */
	TEST_FEATURE ("with comment at end of line");
	strcpy (buf, "frodo test # foo comment\nbilbo test\n");
	pos = 0;
	lineno = 1;

	handler_called = 0;
	last_data = NULL;
	last_file = NULL;
	last_len = 0;
	last_pos = -1;
	last_lineno = 0;

	ret = nih_config_parse_file (buf, strlen (buf), &pos, &lineno,
				     stanzas, &buf);

	TEST_EQ (ret, 0);
	TEST_EQ (pos, 36);

	TEST_EQ (handler_called, 2);
	TEST_EQ_P (last_data, &buf);
	TEST_EQ_P (last_file, buf);
	TEST_EQ (last_len, strlen (buf));
	TEST_EQ (last_pos, 31);
	TEST_EQ (last_lineno, 2);


	/* Check that whitespace at the start of a line is skipped. */
	TEST_FEATURE ("with whitespace at start of line");
	strcpy (buf, "    frodo test\n  \t \t bilbo test\n");
	pos = 0;
	lineno = 1;

	handler_called = 0;
	last_data = NULL;
	last_file = NULL;
	last_len = 0;
	last_pos = -1;
	last_lineno = 0;

	ret = nih_config_parse_file (buf, strlen (buf), &pos, &lineno,
				     stanzas, &buf);

	TEST_EQ (ret, 0);
	TEST_EQ (pos, 32);

	TEST_EQ (handler_called, 2);
	TEST_EQ_P (last_data, &buf);
	TEST_EQ_P (last_file, buf);
	TEST_EQ (last_len, strlen (buf));
	TEST_EQ (last_pos, 27);
	TEST_EQ (last_lineno, 2);


	/* Check that an empty line is skipped over properly. */
	TEST_FEATURE ("with empty line");
	strcpy (buf, "\nfrodo test\nbilbo test\n");
	pos = 0;
	lineno = 1;

	handler_called = 0;
	last_data = NULL;
	last_file = NULL;
	last_len = 0;
	last_pos = -1;
	last_lineno = 0;

	ret = nih_config_parse_file (buf, strlen (buf), &pos, &lineno,
				     stanzas, &buf);

	TEST_EQ (ret, 0);
	TEST_EQ (pos, 23);

	TEST_EQ (handler_called, 2);
	TEST_EQ_P (last_data, &buf);
	TEST_EQ_P (last_file, buf);
	TEST_EQ (last_len, strlen (buf));
	TEST_EQ (last_pos, 18);
	TEST_EQ (last_lineno, 3);


	/* Check that a line containing whitespace is skipped over. */
	TEST_FEATURE ("with line containing only whitespace");
	strcpy (buf, "  \t  \nfrodo test\nbilbo test\n");
	pos = 0;
	lineno = 1;

	handler_called = 0;
	last_data = NULL;
	last_file = NULL;
	last_len = 0;
	last_pos = -1;
	last_lineno = 0;

	ret = nih_config_parse_file (buf, strlen (buf), &pos, &lineno,
				     stanzas, &buf);

	TEST_EQ (ret, 0);
	TEST_EQ (pos, 28);

	TEST_EQ (handler_called, 2);
	TEST_EQ_P (last_data, &buf);
	TEST_EQ_P (last_file, buf);
	TEST_EQ (last_len, strlen (buf));
	TEST_EQ (last_pos, 23);
	TEST_EQ (last_lineno, 3);


	/* Check that a line containing a comment is skipped over. */
	TEST_FEATURE ("with line containing only a comment");
	strcpy (buf, "# hello\nfrodo test\nbilbo test\n");
	pos = 0;
	lineno = 1;

	handler_called = 0;
	last_data = NULL;
	last_file = NULL;
	last_len = 0;
	last_pos = -1;
	last_lineno = 0;

	ret = nih_config_parse_file (buf, strlen (buf), &pos, &lineno,
				     stanzas, &buf);

	TEST_EQ (ret, 0);
	TEST_EQ (pos, 30);

	TEST_EQ (handler_called, 2);
	TEST_EQ_P (last_data, &buf);
	TEST_EQ_P (last_file, buf);
	TEST_EQ (last_len, strlen (buf));
	TEST_EQ (last_pos, 25);
	TEST_EQ (last_lineno, 3);


	/* Check that a line containing a comment after some whitespace
	 * is skipped over.
	 */
	TEST_FEATURE ("with line containing a comment and whitespace");
	strcpy (buf, "  \t  # hello\nfrodo test\nbilbo test\n");
	pos = 0;
	lineno = 1;

	handler_called = 0;
	last_data = NULL;
	last_file = NULL;
	last_len = 0;
	last_pos = -1;
	last_lineno = 0;

	ret = nih_config_parse_file (buf, strlen (buf), &pos, &lineno,
				     stanzas, &buf);

	TEST_EQ (ret, 0);
	TEST_EQ (pos, 35);

	TEST_EQ (handler_called, 2);
	TEST_EQ_P (last_data, &buf);
	TEST_EQ_P (last_file, buf);
	TEST_EQ (last_len, strlen (buf));
	TEST_EQ (last_pos, 30);
	TEST_EQ (last_lineno, 3);


	/* Check that a parser error is raised with the position and line
	 * number set to where it was found.  Only handlers up to that point
	 * should be called.
	 */
	TEST_FEATURE ("with parser error");
	strcpy (buf, "frodo test\n\"bilbo test\n");
	pos = 0;
	lineno = 1;

	handler_called = 0;
	last_data = NULL;
	last_file = NULL;
	last_len = 0;
	last_pos = -1;
	last_lineno = 0;

	ret = nih_config_parse_file (buf, strlen (buf), &pos, &lineno,
				     stanzas, &buf);

	TEST_LT (ret, 0);
	TEST_EQ (pos, 23);
	TEST_EQ (lineno, 3);

	TEST_EQ (handler_called, 1);
	TEST_EQ_P (last_data, &buf);
	TEST_EQ_P (last_file, buf);
	TEST_EQ (last_len, strlen (buf));
	TEST_EQ (last_pos, 6);
	TEST_EQ (last_lineno, 1);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_CONFIG_UNTERMINATED_QUOTE);
	nih_free (err);
}

void
test_parse (void)
{
	FILE     *fd;
	char      filename[PATH_MAX];
	size_t    pos, lineno;
	NihError *err;
	int       ret;

	TEST_FUNCTION ("nih_config_parse");

	/* Check that a file that exists is parsed, with the handlers
	 * called and zero returned.
	 */
	TEST_FEATURE ("with existing file");
	TEST_FILENAME (filename);

	fd = fopen (filename, "w");
	fprintf (fd, "frodo test\n");
	fprintf (fd, "bilbo test\n");
	fclose (fd);

	handler_called = 0;
	last_data = NULL;
	last_file = NULL;
	last_len = 0;
	last_pos = -1;
	last_lineno = 0;

	lineno = 1;

	ret = nih_config_parse (filename, NULL, &lineno, stanzas, &ret);

	TEST_EQ (ret, 0);

	TEST_EQ (handler_called, 2);
	TEST_EQ_P (last_data, &ret);
	TEST_NE_P (last_file, NULL);
	TEST_EQ (last_len, 22);
	TEST_EQ (last_pos, 17);
	TEST_EQ (last_lineno, 2);

	unlink (filename);


	/* Check that an error is raised if the file doesn't exist. */
	TEST_FEATURE ("with non-existant file");
	handler_called = 0;

	ret = nih_config_parse (filename, NULL, NULL, stanzas, &ret);

	TEST_LT (ret, 0);
	TEST_FALSE (handler_called);

	err = nih_error_get ();
	TEST_EQ (err->number, ENOENT);
	nih_free (err);


	/* Check that a parser error is raised with the position and line
	 * number set to where it was found.
	 */
	TEST_FEATURE ("with parser error");
	fd = fopen (filename, "w");
	fprintf (fd, "# first line comment\n");
	fprintf (fd, "\n");
	fprintf (fd, "frodo test\n");
	fprintf (fd, "\"bilbo test\n");
	fprintf (fd, "wibble\n");
	fclose (fd);

	pos = 0;
	lineno = 1;

	handler_called = 0;
	last_data = NULL;
	last_file = NULL;
	last_len = 0;
	last_pos = -1;
	last_lineno = 0;

	ret = nih_config_parse (filename, &pos, &lineno, stanzas, &ret);

	TEST_LT (ret, 0);

	TEST_EQ (handler_called, 1);
	TEST_EQ_P (last_data, &ret);
	TEST_NE_P (last_file, NULL);
	TEST_EQ (last_len, 52);
	TEST_EQ (last_pos, 28);
	TEST_EQ (last_lineno, 3);

	TEST_EQ (pos, 52);
	TEST_EQ (lineno, 6);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_CONFIG_UNTERMINATED_QUOTE);
	nih_free (err);

	unlink (filename);
}


int
main (int   argc,
      char *argv[])
{
	test_has_token ();
	test_token ();
	test_next_token ();
	test_next_arg ();
	test_next_line ();
	test_skip_whitespace ();
	test_skip_comment ();
	test_parse_args ();
	test_parse_command ();
	test_parse_block ();
	test_skip_block ();
	test_parse_stanza ();
	test_parse_file ();
	test_parse ();

	return 0;
}
