/* libnih
 *
 * test_string.c - test suite for nih/string.c
 *
 * Copyright © 2006 Scott James Remnant <scott@netsplit.com>.
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


#include <sys/stat.h>

#include <pty.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <nih/alloc.h>
#include <nih/string.h>


int
test_sprintf (void)
{
	char *str1, *str2;
	int   ret = 0;

	printf ("Testing nih_sprintf()\n");

	printf ("...with no parent\n");
	str1 = nih_sprintf (NULL, "this %s a test %d", "is", 54321);

	/* Returned value should be correct */
	if (strcmp (str1, "this is a test 54321")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Size should be correct */
	if (nih_alloc_size (str1) != strlen ("this is a test 54321") + 1) {
		printf ("BAD: size incorrect.\n");
		ret = 1;
	}

	/* Parent should be none */
	if (nih_alloc_parent (str1) != NULL) {
		printf ("BAD: parent incorrect.\n");
		ret = 1;
	}


	printf ("...with a parent\n");
	str2 = nih_sprintf (str1, "another %d test %s", 12345, "string");

	/* Returned value should be correct */
	if (strcmp (str2, "another 12345 test string")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Size should be correct */
	if (nih_alloc_size (str2)
	    != strlen ("another 12345 test string") + 1) {
		printf ("BAD: size incorrect.\n");
		ret = 1;
	}

	/* Parent should be first string */
	if (nih_alloc_parent (str2) != str1) {
		printf ("BAD: parent incorrect.\n");
		ret = 1;
	}

	nih_free (str1);

	return ret;
}

static char *
my_vsprintf (void *parent,
	     const char *format,
	     ...)
{
	char    *str;
	va_list  args;

	va_start (args, format);
	str = nih_vsprintf (parent, format, args);
	va_end (args);

	return str;
}

int
test_vsprintf (void)
{
	char *str1, *str2;
	int   ret = 0;

	printf ("Testing nih_vsprintf()\n");

	printf ("...with no parent\n");
	str1 = my_vsprintf (NULL, "this %s a test %d", "is", 54321);

	/* Returned value should be correct */
	if (strcmp (str1, "this is a test 54321")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Size should be correct */
	if (nih_alloc_size (str1) != strlen ("this is a test 54321") + 1) {
		printf ("BAD: size incorrect.\n");
		ret = 1;
	}

	/* Parent should be none */
	if (nih_alloc_parent (str1) != NULL) {
		printf ("BAD: parent incorrect.\n");
		ret = 1;
	}


	printf ("...with a parent\n");
	str2 = my_vsprintf (str1, "another %d test %s", 12345, "string");

	/* Returned value should be correct */
	if (strcmp (str2, "another 12345 test string")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Size should be correct */
	if (nih_alloc_size (str2)
	    != strlen ("another 12345 test string") + 1) {
		printf ("BAD: size incorrect.\n");
		ret = 1;
	}

	/* Parent should be first string */
	if (nih_alloc_parent (str2) != str1) {
		printf ("BAD: parent incorrect.\n");
		ret = 1;
	}

	nih_free (str1);

	return ret;
}

int
test_strdup (void)
{
	char *str1, *str2;
	int   ret = 0;

	printf ("Testing nih_strdup()\n");

	printf ("...with no parent\n");
	str1 = nih_strdup (NULL, "this is a test");

	/* Returned value should be correct */
	if (strcmp (str1, "this is a test")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Size should be correct */
	if (nih_alloc_size (str1) != strlen ("this is a test") + 1) {
		printf ("BAD: size incorrect.\n");
		ret = 1;
	}

	/* Parent should be none */
	if (nih_alloc_parent (str1) != NULL) {
		printf ("BAD: parent incorrect.\n");
		ret = 1;
	}


	printf ("...with a parent\n");
	str2 = nih_strdup (str1, "another test string");

	/* Returned value should be correct */
	if (strcmp (str2, "another test string")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Size should be correct */
	if (nih_alloc_size (str2) != strlen ("another test string") + 1) {
		printf ("BAD: size incorrect.\n");
		ret = 1;
	}

	/* Parent should be first string */
	if (nih_alloc_parent (str2) != str1) {
		printf ("BAD: parent incorrect.\n");
		ret = 1;
	}

	nih_free (str1);

	return ret;
}

int
test_strndup (void)
{
	char *str1, *str2, *str;
	int   ret = 0;

	printf ("Testing nih_strndup()\n");

	printf ("...with no parent\n");
	str1 = nih_strndup (NULL, "this is a test", strlen("this is a test"));

	/* Returned value should be correct */
	if (strcmp (str1, "this is a test")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Size should be correct */
	if (nih_alloc_size (str1) != strlen ("this is a test") + 1) {
		printf ("BAD: size incorrect.\n");
		ret = 1;
	}

	/* Parent should be none */
	if (nih_alloc_parent (str1) != NULL) {
		printf ("BAD: parent incorrect.\n");
		ret = 1;
	}


	printf ("...with a parent\n");
	str2 = nih_strndup (str1, "another test string",
			    strlen("another test string"));

	/* Returned value should be correct */
	if (strcmp (str2, "another test string")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Size should be correct */
	if (nih_alloc_size (str2) != strlen ("another test string") + 1) {
		printf ("BAD: size incorrect.\n");
		ret = 1;
	}

	/* Parent should be first string */
	if (nih_alloc_parent (str2) != str1) {
		printf ("BAD: parent incorrect.\n");
		ret = 1;
	}

	nih_free (str1);


	printf ("...with smaller length than string\n");
	str = nih_strndup (NULL, "something to test with", 9);

	/* Returned value should be correct */
	if (strcmp (str, "something")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Size should be correct */
	if (nih_alloc_size (str) != 10) {
		printf ("BAD: size incorrect.\n");
		ret = 1;
	}

	/* Parent should be none */
	if (nih_alloc_parent (str) != NULL) {
		printf ("BAD: parent incorrect.\n");
		ret = 1;
	}

	nih_free (str);


	printf ("...with larger length than string\n");
	str = nih_strndup (NULL, "small string", 20);

	/* Returned value should be correct */
	if (strcmp (str, "small string")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Size should be correct */
	if (nih_alloc_size (str) != 21) {
		printf ("BAD: size incorrect.\n");
		ret = 1;
	}

	/* Parent should be none */
	if (nih_alloc_parent (str) != NULL) {
		printf ("BAD: parent incorrect.\n");
		ret = 1;
	}

	nih_free (str);

	return ret;
}


int
test_str_split (void)
{
	char **array;
	int    ret = 0, i;

	printf ("Testing nih_str_split()\n");

	printf ("...with no repeat\n");
	array = nih_str_split (NULL, "this is  a\ttest", " \t", FALSE);

	/* Check elements; crash means a fail */
	if (strcmp (array[0], "this")
	    || strcmp (array[1], "is")
	    || strcmp (array[2], "")
	    || strcmp (array[3], "a")
	    || strcmp (array[4], "test")) {
		printf ("BAD: array element wasn't what we expected.\n");
		ret = 1;
	}

	/* Last element should be NULL */
	if (array[5] != NULL) {
		printf ("BAD: last array element wasn't NULL.\n");
		ret = 1;
	}

	/* Should have been allocated with nih_alloc */
	if (nih_alloc_size (array) != sizeof (char *) * 6) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}

	/* Strings should have been allocated as children of parent */
	for (i = 0; i < 5; i++) {
		if (nih_alloc_parent (array[i]) != array) {
			printf ("BAD: nih_alloc of string not parent.\n");
			ret = 1;
		}
	}

	nih_free (array);


	printf ("...with repeat\n");
	array = nih_str_split (NULL, "this is  a\ttest", " \t", TRUE);

	/* Check elements; crash means a fail */
	if (strcmp (array[0], "this")
	    || strcmp (array[1], "is")
	    || strcmp (array[2], "a")
	    || strcmp (array[3], "test")) {
		printf ("BAD: array element wasn't what we expected.\n");
		ret = 1;
	}

	/* Last element should be NULL */
	if (array[4] != NULL) {
		printf ("BAD: last array element wasn't NULL.\n");
		ret = 1;
	}

	/* Should have been allocated with nih_alloc */
	if (nih_alloc_size (array) != sizeof (char *) * 5) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}

	/* Strings should have been allocated as children of parent */
	for (i = 0; i < 4; i++) {
		if (nih_alloc_parent (array[i]) != array) {
			printf ("BAD: nih_alloc of string not parent.\n");
			ret = 1;
		}
	}

	nih_free (array);


	printf ("...with empty string\n");
	array = nih_str_split (NULL, "", " ", FALSE);

	/* Only element should be NULL */
	if (array[0] != NULL) {
		printf ("BAD: last array element wasn't NULL.\n");
		ret = 1;
	}

	/* Should have been allocated with nih_alloc */
	if (nih_alloc_size (array) != sizeof (char *) * 1) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}

	nih_free (array);

	return ret;
}

int
test_strv_free (void)
{
	char **strv;
	int    ret = 0;

	printf ("Testing nih_strv_free()\n");
	strv = malloc (sizeof (char *) * 5);
	strv[0] = strdup ("This");
	strv[1] = strdup ("is");
	strv[2] = strdup ("a");
	strv[3] = strdup ("test");
	strv[4] = NULL;

	/* If it doesn't crash, it's a pass */
	nih_strv_free (strv);
	free (strv);

	return ret;
}


int
test_str_wrap (void)
{
	char *str;
	int   ret = 0;

	printf ("Testing nih_str_wrap()\n");

	printf ("...with no wrapping\n");
	str = nih_str_wrap (NULL, "this is a test", 80, 0, 0);

	/* Check returned string */
	if (strcmp (str, "this is a test")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (str);


	printf ("...with embedded newlines\n");
	str = nih_str_wrap (NULL, "this is\na test", 80, 0, 0);

	/* Check returned string */
	if (strcmp (str, "this is\na test")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (str);


	printf ("...with no wrapping and indent\n");
	str = nih_str_wrap (NULL, "this is a test", 80, 2, 0);

	/* Check returned string */
	if (strcmp (str, "  this is a test")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (str);


	printf ("...with embedded newlines and indent\n");
	str = nih_str_wrap (NULL, "this is\na test", 80, 4, 2);

	/* Check returned string */
	if (strcmp (str, "    this is\n  a test")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (str);


	printf ("...with simple wrapping\n");
	str = nih_str_wrap (NULL, "this is an example of a string that will "
			    "need wrapping to fit the line length we set",
			    20, 0, 0);

	/* Check returned string */
	if (strcmp (str, ("this is an example\n"
			  "of a string that\n"
			  "will need wrapping\n"
			  "to fit the line\n"
			  "length we set"))) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (str);


	printf ("...with wrapping and indents\n");
	str = nih_str_wrap (NULL, "this is an example of a string that will "
			    "need wrapping to fit the line length we set",
			    20, 4, 2);

	/* Check returned string */
	if (strcmp (str, ("    this is an\n"
			  "  example of a\n"
			  "  string that will\n"
			  "  need wrapping to\n"
			  "  fit the line\n"
			  "  length we set"))) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (str);


	printf ("...with split inside word\n");
	str = nih_str_wrap (NULL, ("this string is supercalifragilisticexpi"
				   "alidocious even though the sound of it "
				   "is something quite atrocious"), 30, 0, 0);

	/* Check returned string */
	if (strcmp (str, ("this string is\n"
			  "supercalifragilisticexpialidoc\n"
			  "ious even though the sound of\n"
			  "it is something quite\n"
			  "atrocious"))) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (str);


	printf ("...with split inside word and indents\n");
	str = nih_str_wrap (NULL, ("this string is supercalifragilisticexpi"
				   "alidocious even though the sound of it "
				   "is something quite atrocious"), 30, 4, 2);

	/* Check returned string */
	if (strcmp (str, ("    this string is\n"
			  "  supercalifragilisticexpialid\n"
			  "  ocious even though the sound\n"
			  "  of it is something quite\n"
			  "  atrocious"))) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (str);


	return ret;
}


int
test_str_screen_width (void)
{
	struct winsize  winsize;
	size_t          len;
	int             oldstdout, pty, pts, ret = 0;

	printf ("Testing nih_str_screen_width()\n");
	oldstdout = dup (STDOUT_FILENO);

	unsetenv ("COLUMNS");

	winsize.ws_row = 24;
	winsize.ws_col = 40;
	winsize.ws_xpixel = 0;
	winsize.ws_ypixel = 0;
	assert (openpty (&pty, &pts, NULL, NULL, &winsize) == 0);

	printf ("...with screen width\n");
	dup2 (pts, STDOUT_FILENO);
	len = nih_str_screen_width ();
	dup2 (oldstdout, STDOUT_FILENO);

	/* Check return value */
	if (len != 40) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}


	printf ("...with COLUMNS variable\n");
	putenv ("COLUMNS=30");
	dup2 (pts, STDOUT_FILENO);
	len = nih_str_screen_width ();
	dup2 (oldstdout, STDOUT_FILENO);

	/* Check return value */
	if (len != 30) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	unsetenv ("COLUMNS");
	close (pts);
	close (pty);


	assert ((pts = open ("/dev/null", O_RDWR | O_NOCTTY)) >= 0);

	printf ("...with fallback to 80 columns\n");
	dup2 (pts, STDOUT_FILENO);
	len = nih_str_screen_width ();
	dup2 (oldstdout, STDOUT_FILENO);

	/* Check return value */
	if (len != 80) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	close (pts);
	close (oldstdout);

	return ret;
}

int
test_str_screen_wrap (void)
{
	char           *str;
	struct winsize  winsize;
	int             oldstdout, pty, pts, ret = 0;

	printf ("Testing nih_str_screen_wrap()\n");
	oldstdout = dup (STDOUT_FILENO);

	unsetenv ("COLUMNS");

	winsize.ws_row = 24;
	winsize.ws_col = 40;
	winsize.ws_xpixel = 0;
	winsize.ws_ypixel = 0;
	assert (openpty (&pty, &pts, NULL, NULL, &winsize) == 0);

	printf ("...with screen width\n");
	dup2 (pts, STDOUT_FILENO);
	str = nih_str_screen_wrap (NULL, ("this is a string that should need "
					  "wrapping at any different screen "
					  "width that we choose to set"),
				   0, 0);
	dup2 (oldstdout, STDOUT_FILENO);

	/* Check returned string */
	if (strcmp (str, ("this is a string that should need\n"
			  "wrapping at any different screen width\n"
			  "that we choose to set"))) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (str);


	printf ("...with COLUMNS variable\n");
	putenv ("COLUMNS=30");
	dup2 (pts, STDOUT_FILENO);
	str = nih_str_screen_wrap (NULL, ("this is a string that should need "
					  "wrapping at any different screen "
					  "width that we choose to set"),
				   0, 0);
	dup2 (oldstdout, STDOUT_FILENO);

	/* Check returned string */
	if (strcmp (str, ("this is a string that should\n"
			  "need wrapping at any\n"
			  "different screen width that\n"
			  "we choose to set"))) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (str);

	unsetenv ("COLUMNS");
	close (pts);
	close (pty);


	assert ((pts = open ("/dev/null", O_RDWR | O_NOCTTY)) >= 0);

	printf ("...with fallback to 80 columns\n");
	dup2 (pts, STDOUT_FILENO);
	str = nih_str_screen_wrap (NULL, ("this is a string that should need "
					  "wrapping at any different screen "
					  "width that we choose to set"),
				   0, 0);
	dup2 (oldstdout, STDOUT_FILENO);

	/* Check returned string */
	if (strcmp (str, ("this is a string that should need wrapping at "
			  "any different screen width that\n"
			  "we choose to set"))) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (str);

	close (pts);
	close (oldstdout);

	return ret;
}


int
main (int   argc,
      char *argv[])
{
	int ret = 0;

	ret |= test_sprintf ();
	ret |= test_vsprintf ();
	ret |= test_strdup ();
	ret |= test_strndup ();
	ret |= test_str_split ();
	ret |= test_strv_free ();
	ret |= test_str_wrap ();
	ret |= test_str_screen_width ();
	ret |= test_str_screen_wrap ();

	return ret;
}
