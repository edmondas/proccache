/*-
 * Copyright (c) 2009 Edmondas Girkantas <eg@fbsd.lt>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <glib.h> 
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>

#define MAX_BYTES 512

static gchar **args;

/*static gboolean verbose = FALSE;*/

static GOptionEntry entries[] =
{
	/*{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Be verbose", NULL },*/
	{ G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_STRING_ARRAY, &args, NULL, N_("proc [parameter=value] [iname=]file.pc") },
	{ NULL }
};

int main(int argc, char *argv[]) {
	
	gchar *cache_dir = NULL;
	mode_t	cache_dir_mask = 0755;

	gchar *base_name = NULL;
	gchar *cache_path = NULL;
	gchar *cache_file_path = NULL;
	gchar *prog_name = "proccache";
	gchar *file_name = NULL;
	gchar *cfile_name = NULL;
	gchar *cmd = NULL;
	gchar tmp_buf[21] = ""; /* size of unsigned long long + 1 */
	
	gchar first_char[2] = "";
	gchar second_char[2] = "";
	gchar *first_path = NULL;
	gchar *second_path = NULL;
	gchar **elems;

	guint index;
	guint index_count = 0;
	gboolean ret;
	gint status;
	
	struct stat file_stat;
	gchar *contents = NULL;
	gsize file_size = 0;
	
	char buf[MAX_BYTES];
	size_t count;
	FILE *in, *out;
	
	gchar *params = NULL;
	gchar *tmp = NULL;
	const gchar *checksum = NULL;
	
	GError *error = NULL;
	GOptionContext *context = NULL;
	
	GChecksum* md5sum = NULL;
	
	g_set_prgname(prog_name);
	
	context = g_option_context_new ("- tool for caching Pro*C precompiler output");
	g_option_context_add_main_entries (context, entries, NULL);
	
	if (!g_option_context_parse (context, &argc, &argv, &error))
	{
		g_print("option parsing failed: %s\n", error->message);
		g_option_context_free(context);
		return(1);
	}

	g_option_context_free(context);
    
	/* check if there are arguments */
	if(!args){
		g_print("Try %s --help for more information\n", g_get_prgname());
		return(1);
	}
    
	/* check parameters if they are correct */
	for (index = 0; args[index] != NULL; index++) {
		if (index == 0) {
			/* accept only proc as first parameter */
			base_name = g_path_get_basename(args[0]);
			if (g_strcmp0(base_name, "proc") != 0) {
				g_print("First parameter should be Pro*C preprocessor!\n");
				g_free(base_name);
				return(1);
			}
			g_free(base_name);
		}
		index_count++;
	}
	
	/* check if we have enough parameters */
	if (index_count < 2) {
		g_print("Try %s --help for more information\n", g_get_prgname()); 		
		return(1);
	}	
	
	/* get user's cache directory */
	cache_dir = (gchar *)g_get_user_cache_dir();
	if (!cache_dir) {
		g_print("Couldn't get cache directory for a user\n");
		return(1);
	}

	/* build full path for cache */
	cache_path = g_build_path(G_DIR_SEPARATOR_S, cache_dir, prog_name, NULL);

	/* create new or use existing directory for cache */
	if (g_file_test(cache_path, G_FILE_TEST_EXISTS)) {
		if (g_file_test(cache_path, G_FILE_TEST_IS_REGULAR)) {
			g_print("Cache directory %s is file\n", cache_path);
			g_free(cache_dir);
			g_free(cache_path);
			return(1);
		}
	} else {
		if(g_mkdir_with_parents(cache_path, cache_dir_mask) == -1) {
			g_print("Couldn't create cache directory\n");
			g_free(cache_dir);
			g_free(cache_path);
			return(1);
		}
	}

	/* process arguments */
	for (index = 1; args[index] != NULL; index++) {
		/* file name can be passed by iname parameter */
		if (g_str_has_prefix(args[index], "iname=")) {
			file_name = g_strdup(args[index]+6);
		}
		
		/* check if argument has = symbol */
		else if (strchr(args[index], '=')) {
			/* build string with all arguments without file name */
			if (params == NULL) {
				params = g_strdup(args[index]);
			} else {
				tmp = g_strjoin(" ",params, args[index], NULL);
				g_free(params);
				params = g_strdup(tmp);
				g_free(tmp);  
			}
		} else {
			file_name = g_strdup(args[index]);
		}
	}

	/* check if file has .pc extension otherwise add it */
	if (!g_str_has_suffix(file_name,".pc")) {
		(void) g_strlcat(file_name, ".pc", (gsize)strlen(file_name)+4);
	}

	/* build command line */
	cmd = g_strjoinv(" ", args);

	/* start computing of checksum */	
	md5sum = g_checksum_new(G_CHECKSUM_MD5);

	/* add parameters to checksum */
	if (params) {
		g_checksum_update(md5sum, (guchar *)params, strlen(params));
	}

	/* add file to checksum */
	ret = g_file_get_contents(file_name, &contents, &file_size, NULL);
	g_checksum_update(md5sum, (guchar *)contents, file_size);

	/* add file's mtime to checksum */
	status = g_stat(file_name, &file_stat);
	(void) g_snprintf(tmp_buf, 11, "%ld", (long int)file_stat.st_mtime);
	g_checksum_update(md5sum, (guchar *)tmp_buf, strlen(tmp_buf));

	/* add file's size to checksum */
	(void) g_snprintf(tmp_buf, 21, "%llu", (long long unsigned int)file_size);
	g_checksum_update(md5sum, (guchar *)tmp_buf, strlen(tmp_buf));	

	/* create a checksum */
	checksum = g_checksum_get_string(md5sum);
	
	/* check if file is in cache */
	(void) g_sprintf(first_char, "%c", checksum[0]);
	(void) g_sprintf(second_char, "%c", checksum[1]);
	
	cache_file_path = g_build_path("/", cache_path, first_char, second_char, checksum, NULL);
	
	/* build a path for c file */
	elems = g_strsplit(file_name, ".", 0);
	cfile_name = g_build_path(".", elems[0], "c", NULL);
	g_strfreev(elems);	
	
	if (g_file_test(cache_file_path, G_FILE_TEST_EXISTS)) {
		
		/* copy file from cache */
		if (g_file_test(cache_file_path, G_FILE_TEST_EXISTS)) {
			in = fopen(cache_file_path, "r");
			out = fopen(cfile_name, "w");

			while(!feof(in)) {
				count = fread(buf, 1, MAX_BYTES, in);
				(void) fwrite(buf, 1, count, out); 
			}

			(void) fclose(out);			
			(void) fclose(in);
		}
	} else {
		/* execute ProC precompiler */
		ret = g_spawn_command_line_sync( cmd, NULL, NULL, NULL, NULL);
		
		if (!ret) {
			(void) g_printf("Failed to run Pro*C precompiler\n");

			g_checksum_free(md5sum);	
			if (params) {
				g_free(params);
			}
			
/*			g_free((gchar *)checksum);*/
			g_free(cmd);
			g_free(cfile_name);
			g_free(cache_file_path);
			g_free(cache_dir);
			g_free(cache_path);
			
			return 1;
		}
		
		/* create N levels directories to keep directories small */
		first_path = g_build_path("/", cache_path, first_char, NULL);
		second_path = g_build_path("/", first_path, second_char, NULL);
		
		if(!g_file_test(first_path, G_FILE_TEST_EXISTS)) {
			/*TODO: check if directory was created */
			(void) g_mkdir(first_path, cache_dir_mask);
		}

		if(!g_file_test(second_path, G_FILE_TEST_EXISTS)) {
			/*TODO: check if directory was created */
			(void) g_mkdir(second_path, cache_dir_mask);
		}

		/* copy a file to cache */
		if (g_file_test(cfile_name, G_FILE_TEST_EXISTS)) {
			in = fopen(cfile_name, "r");
			out = fopen(cache_file_path, "w");

			while(!feof(in)) {
				count = fread(buf, 1, MAX_BYTES, in);
				(void) fwrite(buf, 1, count, out);
			}

			(void) fclose(out);
			(void) fclose(in);
		}
	}

	/* free memory */
	g_checksum_free(md5sum);	
	if (params) {
		g_free(params);
	}

/*	g_free((gchar *)checksum);*/
	g_free(cmd);
	g_free(cfile_name);
	g_free(cache_file_path);
	g_free(cache_dir);
	g_free(cache_path);

	return(0);
}
