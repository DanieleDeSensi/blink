/*
  File autogenerated by gengetopt version 2.22.1
  generated with the following command:
  gengetopt -S -i netgauge.ggo -F netgauge_cmdline -f netgauge_parser -a netgauge_cmd_struct --no-help 

  The developers of gengetopt consider the fixed text that goes in all
  gengetopt output files to be in the public domain:
  we make no copyright claims on it.
*/

/* If we use autoconf.  */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getopt.h"

#include "netgauge_cmdline.h"

const char *netgauge_cmd_struct_purpose = "";

const char *netgauge_cmd_struct_usage = "Usage: netgauge [OPTIONS]...";

const char *netgauge_cmd_struct_description = "";

const char *netgauge_cmd_struct_help[] = {
  "  -V, --version              Print version and exit",
  "  -h, --help                 print help  (default=off)",
  "  -v, --verbosity=INT        verbosity level  (default=`1')",
  "  -a, --server               act as server  (default=off)",
  "  -o, --output=STRING        output file name  (default=`ng.out')",
  "  -f, --full-output=STRING   output file name for all measurements  \n                               (default=`ng-full.out')",
  "  -c, --tests=INT            testcount  (default=`100')",
  "      --hostnames            print hostnames  (default=off)",
  "  -t, --time=INT             max time per test (s)  (default=`100')",
  "  -s, --size=STRING          datasize (bytes, from-to)  (default=`1-131072')",
  "  -m, --mode=STRING          transmission mode  (default=`mpi')",
  "  -x, --comm_pattern=STRING  communication pattern  (default=`one_one')",
  "  -g, --grad=INT             grade of geometrical size distanced  (default=`2')",
  "  -w, --manpage              write manpage to stdout  (default=off)",
  "  -i, --init-thread          initialize with MPI_THREAD_MULTIPLE instead of \n                               MPI_THREAD_SINGLE  (default=off)",
  "  -q, --sanity-check         perform sanity check of timer  (default=off)",
    0
};

typedef enum {ARG_NO
  , ARG_FLAG
  , ARG_STRING
  , ARG_INT
} netgauge_parser_arg_type;

static
void clear_given (struct netgauge_cmd_struct *args_info);
static
void clear_args (struct netgauge_cmd_struct *args_info);

static int
netgauge_parser_internal (int argc, char * const *argv, struct netgauge_cmd_struct *args_info,
                        struct netgauge_parser_params *params, const char *additional_error);

struct line_list
{
  char * string_arg;
  struct line_list * next;
};

static struct line_list *cmd_line_list = 0;
static struct line_list *cmd_line_list_tmp = 0;

static void
free_cmd_list(void)
{
  /* free the list of a previous call */
  if (cmd_line_list)
    {
      while (cmd_line_list) {
        cmd_line_list_tmp = cmd_line_list;
        cmd_line_list = cmd_line_list->next;
        free (cmd_line_list_tmp->string_arg);
        free (cmd_line_list_tmp);
      }
    }
}


static char *
gengetopt_strdup (const char *s);

static
void clear_given (struct netgauge_cmd_struct *args_info)
{
  args_info->version_given = 0 ;
  args_info->help_given = 0 ;
  args_info->verbosity_given = 0 ;
  args_info->server_given = 0 ;
  args_info->output_given = 0 ;
  args_info->full_output_given = 0 ;
  args_info->tests_given = 0 ;
  args_info->hostnames_given = 0 ;
  args_info->time_given = 0 ;
  args_info->size_given = 0 ;
  args_info->mode_given = 0 ;
  args_info->comm_pattern_given = 0 ;
  args_info->grad_given = 0 ;
  args_info->manpage_given = 0 ;
  args_info->init_thread_given = 0 ;
  args_info->sanity_check_given = 0 ;
}

static
void clear_args (struct netgauge_cmd_struct *args_info)
{
  args_info->help_flag = 0;
  args_info->verbosity_arg = 1;
  args_info->verbosity_orig = NULL;
  args_info->server_flag = 0;
  args_info->output_arg = gengetopt_strdup ("ng.out");
  args_info->output_orig = NULL;
  args_info->full_output_arg = gengetopt_strdup ("ng-full.out");
  args_info->full_output_orig = NULL;
  args_info->tests_arg = 100;
  args_info->tests_orig = NULL;
  args_info->hostnames_flag = 0;
  args_info->time_arg = 100;
  args_info->time_orig = NULL;
  args_info->size_arg = gengetopt_strdup ("1-131072");
  args_info->size_orig = NULL;
  args_info->mode_arg = gengetopt_strdup ("mpi");
  args_info->mode_orig = NULL;
  args_info->comm_pattern_arg = gengetopt_strdup ("one_one");
  args_info->comm_pattern_orig = NULL;
  args_info->grad_arg = 2;
  args_info->grad_orig = NULL;
  args_info->manpage_flag = 0;
  args_info->init_thread_flag = 0;
  args_info->sanity_check_flag = 0;
  
}

static
void init_args_info(struct netgauge_cmd_struct *args_info)
{


  args_info->version_help = netgauge_cmd_struct_help[0] ;
  args_info->help_help = netgauge_cmd_struct_help[1] ;
  args_info->verbosity_help = netgauge_cmd_struct_help[2] ;
  args_info->server_help = netgauge_cmd_struct_help[3] ;
  args_info->output_help = netgauge_cmd_struct_help[4] ;
  args_info->full_output_help = netgauge_cmd_struct_help[5] ;
  args_info->tests_help = netgauge_cmd_struct_help[6] ;
  args_info->hostnames_help = netgauge_cmd_struct_help[7] ;
  args_info->time_help = netgauge_cmd_struct_help[8] ;
  args_info->size_help = netgauge_cmd_struct_help[9] ;
  args_info->mode_help = netgauge_cmd_struct_help[10] ;
  args_info->comm_pattern_help = netgauge_cmd_struct_help[11] ;
  args_info->grad_help = netgauge_cmd_struct_help[12] ;
  args_info->manpage_help = netgauge_cmd_struct_help[13] ;
  args_info->init_thread_help = netgauge_cmd_struct_help[14] ;
  args_info->sanity_check_help = netgauge_cmd_struct_help[15] ;
  
}

void
netgauge_parser_print_version (void)
{
  printf ("%s %s\n", NETGAUGE_PARSER_PACKAGE, NETGAUGE_PARSER_VERSION);
}

static void print_help_common(void) {
  netgauge_parser_print_version ();

  if (strlen(netgauge_cmd_struct_purpose) > 0)
    printf("\n%s\n", netgauge_cmd_struct_purpose);

  if (strlen(netgauge_cmd_struct_usage) > 0)
    printf("\n%s\n", netgauge_cmd_struct_usage);

  printf("\n");

  if (strlen(netgauge_cmd_struct_description) > 0)
    printf("%s\n\n", netgauge_cmd_struct_description);
}

void
netgauge_parser_print_help (void)
{
  int i = 0;
  print_help_common();
  while (netgauge_cmd_struct_help[i])
    printf("%s\n", netgauge_cmd_struct_help[i++]);
}

void
netgauge_parser_init (struct netgauge_cmd_struct *args_info)
{
  clear_given (args_info);
  clear_args (args_info);
  init_args_info (args_info);
}

void
netgauge_parser_params_init(struct netgauge_parser_params *params)
{
  if (params)
    { 
      params->override = 0;
      params->initialize = 1;
      params->check_required = 1;
      params->check_ambiguity = 0;
      params->print_errors = 1;
    }
}

struct netgauge_parser_params *
netgauge_parser_params_create(void)
{
  struct netgauge_parser_params *params = 
    (struct netgauge_parser_params *)malloc(sizeof(struct netgauge_parser_params));
  netgauge_parser_params_init(params);  
  return params;
}

static void
free_string_field (char **s)
{
  if (*s)
    {
      free (*s);
      *s = 0;
    }
}


static void
netgauge_parser_release (struct netgauge_cmd_struct *args_info)
{

  free_string_field (&(args_info->verbosity_orig));
  free_string_field (&(args_info->output_arg));
  free_string_field (&(args_info->output_orig));
  free_string_field (&(args_info->full_output_arg));
  free_string_field (&(args_info->full_output_orig));
  free_string_field (&(args_info->tests_orig));
  free_string_field (&(args_info->time_orig));
  free_string_field (&(args_info->size_arg));
  free_string_field (&(args_info->size_orig));
  free_string_field (&(args_info->mode_arg));
  free_string_field (&(args_info->mode_orig));
  free_string_field (&(args_info->comm_pattern_arg));
  free_string_field (&(args_info->comm_pattern_orig));
  free_string_field (&(args_info->grad_orig));
  
  

  clear_given (args_info);
}


static void
write_into_file(FILE *outfile, const char *opt, const char *arg, char *values[])
{
  if (arg) {
    fprintf(outfile, "%s=\"%s\"\n", opt, arg);
  } else {
    fprintf(outfile, "%s\n", opt);
  }
}


int
netgauge_parser_dump(FILE *outfile, struct netgauge_cmd_struct *args_info)
{
  int i = 0;

  if (!outfile)
    {
      fprintf (stderr, "%s: cannot dump options to stream\n", NETGAUGE_PARSER_PACKAGE);
      return EXIT_FAILURE;
    }

  if (args_info->version_given)
    write_into_file(outfile, "version", 0, 0 );
  if (args_info->help_given)
    write_into_file(outfile, "help", 0, 0 );
  if (args_info->verbosity_given)
    write_into_file(outfile, "verbosity", args_info->verbosity_orig, 0);
  if (args_info->server_given)
    write_into_file(outfile, "server", 0, 0 );
  if (args_info->output_given)
    write_into_file(outfile, "output", args_info->output_orig, 0);
  if (args_info->full_output_given)
    write_into_file(outfile, "full-output", args_info->full_output_orig, 0);
  if (args_info->tests_given)
    write_into_file(outfile, "tests", args_info->tests_orig, 0);
  if (args_info->hostnames_given)
    write_into_file(outfile, "hostnames", 0, 0 );
  if (args_info->time_given)
    write_into_file(outfile, "time", args_info->time_orig, 0);
  if (args_info->size_given)
    write_into_file(outfile, "size", args_info->size_orig, 0);
  if (args_info->mode_given)
    write_into_file(outfile, "mode", args_info->mode_orig, 0);
  if (args_info->comm_pattern_given)
    write_into_file(outfile, "comm_pattern", args_info->comm_pattern_orig, 0);
  if (args_info->grad_given)
    write_into_file(outfile, "grad", args_info->grad_orig, 0);
  if (args_info->manpage_given)
    write_into_file(outfile, "manpage", 0, 0 );
  if (args_info->init_thread_given)
    write_into_file(outfile, "init-thread", 0, 0 );
  if (args_info->sanity_check_given)
    write_into_file(outfile, "sanity-check", 0, 0 );
  

  i = EXIT_SUCCESS;
  return i;
}

int
netgauge_parser_file_save(const char *filename, struct netgauge_cmd_struct *args_info)
{
  FILE *outfile;
  int i = 0;

  outfile = fopen(filename, "w");

  if (!outfile)
    {
      fprintf (stderr, "%s: cannot open file for writing: %s\n", NETGAUGE_PARSER_PACKAGE, filename);
      return EXIT_FAILURE;
    }

  i = netgauge_parser_dump(outfile, args_info);
  fclose (outfile);

  return i;
}

void
netgauge_parser_free (struct netgauge_cmd_struct *args_info)
{
  netgauge_parser_release (args_info);
}

/** @brief replacement of strdup, which is not standard */
char *
gengetopt_strdup (const char *s)
{
  char *result = NULL;
  if (!s)
    return result;

  result = (char*)malloc(strlen(s) + 1);
  if (result == (char*)0)
    return (char*)0;
  strcpy(result, s);
  return result;
}

int
netgauge_parser (int argc, char * const *argv, struct netgauge_cmd_struct *args_info)
{
  return netgauge_parser2 (argc, argv, args_info, 0, 1, 1);
}

int
netgauge_parser_ext (int argc, char * const *argv, struct netgauge_cmd_struct *args_info,
                   struct netgauge_parser_params *params)
{
  int result;
  result = netgauge_parser_internal (argc, argv, args_info, params, NULL);

  if (result == EXIT_FAILURE)
    {
      netgauge_parser_free (args_info);
      exit (EXIT_FAILURE);
    }
  
  return result;
}

int
netgauge_parser2 (int argc, char * const *argv, struct netgauge_cmd_struct *args_info, int override, int initialize, int check_required)
{
  int result;
  struct netgauge_parser_params params;
  
  params.override = override;
  params.initialize = initialize;
  params.check_required = check_required;
  params.check_ambiguity = 0;
  params.print_errors = 1;

  result = netgauge_parser_internal (argc, argv, args_info, &params, NULL);

  if (result == EXIT_FAILURE)
    {
      netgauge_parser_free (args_info);
      exit (EXIT_FAILURE);
    }
  
  return result;
}

int
netgauge_parser_required (struct netgauge_cmd_struct *args_info, const char *prog_name)
{
  return EXIT_SUCCESS;
}


static char *package_name = 0;

/**
 * @brief updates an option
 * @param field the generic pointer to the field to update
 * @param orig_field the pointer to the orig field
 * @param field_given the pointer to the number of occurrence of this option
 * @param prev_given the pointer to the number of occurrence already seen
 * @param value the argument for this option (if null no arg was specified)
 * @param possible_values the possible values for this option (if specified)
 * @param default_value the default value (in case the option only accepts fixed values)
 * @param arg_type the type of this option
 * @param check_ambiguity @see netgauge_parser_params.check_ambiguity
 * @param override @see netgauge_parser_params.override
 * @param no_free whether to free a possible previous value
 * @param multiple_option whether this is a multiple option
 * @param long_opt the corresponding long option
 * @param short_opt the corresponding short option (or '-' if none)
 * @param additional_error possible further error specification
 */
static
int update_arg(void *field, char **orig_field,
               unsigned int *field_given, unsigned int *prev_given, 
               char *value, char *possible_values[], const char *default_value,
               netgauge_parser_arg_type arg_type,
               int check_ambiguity, int override,
               int no_free, int multiple_option,
               const char *long_opt, char short_opt,
               const char *additional_error)
{
  char *stop_char = 0;
  const char *val = value;
  int found;
  char **string_field;

  stop_char = 0;
  found = 0;

  if (!multiple_option && prev_given && (*prev_given || (check_ambiguity && *field_given)))
    {
      if (short_opt != '-')
        fprintf (stderr, "%s: `--%s' (`-%c') option given more than once%s\n", 
               package_name, long_opt, short_opt,
               (additional_error ? additional_error : ""));
      else
        fprintf (stderr, "%s: `--%s' option given more than once%s\n", 
               package_name, long_opt,
               (additional_error ? additional_error : ""));
      return 1; /* failure */
    }

    
  if (field_given && *field_given && ! override)
    return 0;
  if (prev_given)
    (*prev_given)++;
  if (field_given)
    (*field_given)++;
  if (possible_values)
    val = possible_values[found];

  switch(arg_type) {
  case ARG_FLAG:
    *((int *)field) = !*((int *)field);
    break;
  case ARG_INT:
    if (val) *((int *)field) = strtol (val, &stop_char, 0);
    break;
  case ARG_STRING:
    if (val) {
      string_field = (char **)field;
      if (!no_free && *string_field)
        free (*string_field); /* free previous string */
      *string_field = gengetopt_strdup (val);
    }
    break;
  default:
    break;
  };

  /* check numeric conversion */
  switch(arg_type) {
  case ARG_INT:
    if (val && !(stop_char && *stop_char == '\0')) {
      fprintf(stderr, "%s: invalid numeric value: %s\n", package_name, val);
      return 1; /* failure */
    }
    break;
  default:
    ;
  };

  /* store the original value */
  switch(arg_type) {
  case ARG_NO:
  case ARG_FLAG:
    break;
  default:
    if (value && orig_field) {
      if (no_free) {
        *orig_field = value;
      } else {
        if (*orig_field)
          free (*orig_field); /* free previous string */
        *orig_field = gengetopt_strdup (value);
      }
    }
  };

  return 0; /* OK */
}


int
netgauge_parser_internal (int argc, char * const *argv, struct netgauge_cmd_struct *args_info,
                        struct netgauge_parser_params *params, const char *additional_error)
{
  int c;	/* Character of the parsed option.  */

  int error = 0;
  struct netgauge_cmd_struct local_args_info;
  
  int override;
  int initialize;
  int check_required;
  int check_ambiguity;
  
  package_name = argv[0];
  
  override = params->override;
  initialize = params->initialize;
  check_required = params->check_required;
  check_ambiguity = params->check_ambiguity;

  if (initialize)
    netgauge_parser_init (args_info);

  netgauge_parser_init (&local_args_info);

  optarg = 0;
  optind = 0;
  opterr = params->print_errors;
  optopt = '?';

  while (1)
    {
      int option_index = 0;

      static struct option long_options[] = {
        { "version",	0, NULL, 'V' },
        { "help",	0, NULL, 'h' },
        { "verbosity",	1, NULL, 'v' },
        { "server",	0, NULL, 'a' },
        { "output",	1, NULL, 'o' },
        { "full-output",	1, NULL, 'f' },
        { "tests",	1, NULL, 'c' },
        { "hostnames",	0, NULL, 0 },
        { "time",	1, NULL, 't' },
        { "size",	1, NULL, 's' },
        { "mode",	1, NULL, 'm' },
        { "comm_pattern",	1, NULL, 'x' },
        { "grad",	1, NULL, 'g' },
        { "manpage",	0, NULL, 'w' },
        { "init-thread",	0, NULL, 'i' },
        { "sanity-check",	0, NULL, 'q' },
        { NULL,	0, NULL, 0 }
      };

      c = getopt_long (argc, argv, "Vhv:ao:f:c:t:s:m:x:g:wiq", long_options, &option_index);

      if (c == -1) break;	/* Exit from `while (1)' loop.  */

      switch (c)
        {
        case 'V':	/* Print version and exit.  */
          netgauge_parser_print_version ();
          netgauge_parser_free (&local_args_info);
          exit (EXIT_SUCCESS);

        case 'h':	/* print help.  */
        
        
          if (update_arg((void *)&(args_info->help_flag), 0, &(args_info->help_given),
              &(local_args_info.help_given), optarg, 0, 0, ARG_FLAG,
              check_ambiguity, override, 1, 0, "help", 'h',
              additional_error))
            goto failure;
        
          break;
        case 'v':	/* verbosity level.  */
        
        
          if (update_arg( (void *)&(args_info->verbosity_arg), 
               &(args_info->verbosity_orig), &(args_info->verbosity_given),
              &(local_args_info.verbosity_given), optarg, 0, "1", ARG_INT,
              check_ambiguity, override, 0, 0,
              "verbosity", 'v',
              additional_error))
            goto failure;
        
          break;
        case 'a':	/* act as server.  */
        
        
          if (update_arg((void *)&(args_info->server_flag), 0, &(args_info->server_given),
              &(local_args_info.server_given), optarg, 0, 0, ARG_FLAG,
              check_ambiguity, override, 1, 0, "server", 'a',
              additional_error))
            goto failure;
        
          break;
        case 'o':	/* output file name.  */
        
        
          if (update_arg( (void *)&(args_info->output_arg), 
               &(args_info->output_orig), &(args_info->output_given),
              &(local_args_info.output_given), optarg, 0, "ng.out", ARG_STRING,
              check_ambiguity, override, 0, 0,
              "output", 'o',
              additional_error))
            goto failure;
        
          break;
        case 'f':	/* output file name for all measurements.  */
        
        
          if (update_arg( (void *)&(args_info->full_output_arg), 
               &(args_info->full_output_orig), &(args_info->full_output_given),
              &(local_args_info.full_output_given), optarg, 0, "ng-full.out", ARG_STRING,
              check_ambiguity, override, 0, 0,
              "full-output", 'f',
              additional_error))
            goto failure;
        
          break;
        case 'c':	/* testcount.  */
        
        
          if (update_arg( (void *)&(args_info->tests_arg), 
               &(args_info->tests_orig), &(args_info->tests_given),
              &(local_args_info.tests_given), optarg, 0, "100", ARG_INT,
              check_ambiguity, override, 0, 0,
              "tests", 'c',
              additional_error))
            goto failure;
        
          break;
        case 't':	/* max time per test (s).  */
        
        
          if (update_arg( (void *)&(args_info->time_arg), 
               &(args_info->time_orig), &(args_info->time_given),
              &(local_args_info.time_given), optarg, 0, "100", ARG_INT,
              check_ambiguity, override, 0, 0,
              "time", 't',
              additional_error))
            goto failure;
        
          break;
        case 's':	/* datasize (bytes, from-to).  */
        
        
          if (update_arg( (void *)&(args_info->size_arg), 
               &(args_info->size_orig), &(args_info->size_given),
              &(local_args_info.size_given), optarg, 0, "1-131072", ARG_STRING,
              check_ambiguity, override, 0, 0,
              "size", 's',
              additional_error))
            goto failure;
        
          break;
        case 'm':	/* transmission mode.  */
        
        
          if (update_arg( (void *)&(args_info->mode_arg), 
               &(args_info->mode_orig), &(args_info->mode_given),
              &(local_args_info.mode_given), optarg, 0, "mpi", ARG_STRING,
              check_ambiguity, override, 0, 0,
              "mode", 'm',
              additional_error))
            goto failure;
        
          break;
        case 'x':	/* communication pattern.  */
        
        
          if (update_arg( (void *)&(args_info->comm_pattern_arg), 
               &(args_info->comm_pattern_orig), &(args_info->comm_pattern_given),
              &(local_args_info.comm_pattern_given), optarg, 0, "one_one", ARG_STRING,
              check_ambiguity, override, 0, 0,
              "comm_pattern", 'x',
              additional_error))
            goto failure;
        
          break;
        case 'g':	/* grade of geometrical size distanced.  */
        
        
          if (update_arg( (void *)&(args_info->grad_arg), 
               &(args_info->grad_orig), &(args_info->grad_given),
              &(local_args_info.grad_given), optarg, 0, "2", ARG_INT,
              check_ambiguity, override, 0, 0,
              "grad", 'g',
              additional_error))
            goto failure;
        
          break;
        case 'w':	/* write manpage to stdout.  */
        
        
          if (update_arg((void *)&(args_info->manpage_flag), 0, &(args_info->manpage_given),
              &(local_args_info.manpage_given), optarg, 0, 0, ARG_FLAG,
              check_ambiguity, override, 1, 0, "manpage", 'w',
              additional_error))
            goto failure;
        
          break;
        case 'i':	/* initialize with MPI_THREAD_MULTIPLE instead of MPI_THREAD_SINGLE.  */
        
        
          if (update_arg((void *)&(args_info->init_thread_flag), 0, &(args_info->init_thread_given),
              &(local_args_info.init_thread_given), optarg, 0, 0, ARG_FLAG,
              check_ambiguity, override, 1, 0, "init-thread", 'i',
              additional_error))
            goto failure;
        
          break;
        case 'q':	/* perform sanity check of timer.  */
        
        
          if (update_arg((void *)&(args_info->sanity_check_flag), 0, &(args_info->sanity_check_given),
              &(local_args_info.sanity_check_given), optarg, 0, 0, ARG_FLAG,
              check_ambiguity, override, 1, 0, "sanity-check", 'q',
              additional_error))
            goto failure;
        
          break;

        case 0:	/* Long option with no short option */
          /* print hostnames.  */
          if (strcmp (long_options[option_index].name, "hostnames") == 0)
          {
          
          
            if (update_arg((void *)&(args_info->hostnames_flag), 0, &(args_info->hostnames_given),
                &(local_args_info.hostnames_given), optarg, 0, 0, ARG_FLAG,
                check_ambiguity, override, 1, 0, "hostnames", '-',
                additional_error))
              goto failure;
          
          }
          
          break;
        case '?':	/* Invalid option.  */
          /* `getopt_long' already printed an error message.  */
          goto failure;

        default:	/* bug: option not considered.  */
          fprintf (stderr, "%s: option unknown: %c%s\n", NETGAUGE_PARSER_PACKAGE, c, (additional_error ? additional_error : ""));
          abort ();
        } /* switch */
    } /* while */




  netgauge_parser_release (&local_args_info);

  if ( error )
    return (EXIT_FAILURE);

  return 0;

failure:
  
  netgauge_parser_release (&local_args_info);
  return (EXIT_FAILURE);
}

static unsigned int
netgauge_parser_create_argv(const char *cmdline_, char ***argv_ptr, const char *prog_name)
{
  char *cmdline, *p;
  size_t n = 0, j;
  int i;

  if (prog_name) {
    cmd_line_list_tmp = (struct line_list *) malloc (sizeof (struct line_list));
    cmd_line_list_tmp->next = cmd_line_list;
    cmd_line_list = cmd_line_list_tmp;
    cmd_line_list->string_arg = gengetopt_strdup (prog_name);

    ++n;
  }

  cmdline = gengetopt_strdup(cmdline_);
  p = cmdline;

  while (p && strlen(p))
    {
      j = strcspn(p, " \t");
      ++n;
      if (j && j < strlen(p))
        {
          p[j] = '\0';

          cmd_line_list_tmp = (struct line_list *) malloc (sizeof (struct line_list));
          cmd_line_list_tmp->next = cmd_line_list;
          cmd_line_list = cmd_line_list_tmp;
          cmd_line_list->string_arg = gengetopt_strdup (p);

          p += (j+1);
          p += strspn(p, " \t");
        }
      else
        {
          cmd_line_list_tmp = (struct line_list *) malloc (sizeof (struct line_list));
          cmd_line_list_tmp->next = cmd_line_list;
          cmd_line_list = cmd_line_list_tmp;
          cmd_line_list->string_arg = gengetopt_strdup (p);

          break;
        }
    }

  *argv_ptr = (char **) malloc((n + 1) * sizeof(char *));
  cmd_line_list_tmp = cmd_line_list;
  for (i = (n-1); i >= 0; --i)
    {
      (*argv_ptr)[i] = cmd_line_list_tmp->string_arg;
      cmd_line_list_tmp = cmd_line_list_tmp->next;
    }

  (*argv_ptr)[n] = NULL;

  free(cmdline);
  return n;
}

int
netgauge_parser_string(const char *cmdline, struct netgauge_cmd_struct *args_info, const char *prog_name)
{
  return netgauge_parser_string2(cmdline, args_info, prog_name, 0, 1, 1);
}

int
netgauge_parser_string2(const char *cmdline, struct netgauge_cmd_struct *args_info, const char *prog_name,
    int override, int initialize, int check_required)
{
  struct netgauge_parser_params params;

  params.override = override;
  params.initialize = initialize;
  params.check_required = check_required;
  params.check_ambiguity = 0;
  params.print_errors = 1;

  return netgauge_parser_string_ext(cmdline, args_info, prog_name, &params);
}

int
netgauge_parser_string_ext(const char *cmdline, struct netgauge_cmd_struct *args_info, const char *prog_name,
    struct netgauge_parser_params *params)
{
  char **argv_ptr = 0;
  int result;
  unsigned int argc;
  
  argc = netgauge_parser_create_argv(cmdline, &argv_ptr, prog_name);
  
  result =
    netgauge_parser_internal (argc, argv_ptr, args_info, params, 0);
  
  if (argv_ptr)
    {
      free (argv_ptr);
    }

  free_cmd_list();
  
  if (result == EXIT_FAILURE)
    {
      netgauge_parser_free (args_info);
      exit (EXIT_FAILURE);
    }
  
  return result;
}

