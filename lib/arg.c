/* arg.c - argument parser */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2007,2008  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/term.h>
#include <grub/extcmd.h>
#include <grub/i18n.h>

GRUB_EXPORT(grub_arg_show_help);

/* Built-in parser for default options.  */
#define SHORT_ARG_HELP	-100
#define SHORT_ARG_USAGE	-101

static const struct grub_arg_option help_options[] =
  {
    {"help", SHORT_ARG_HELP, 0,
     N_("Display this help and exit."), 0, ARG_TYPE_NONE},
    {"usage", SHORT_ARG_USAGE, 0,
     N_("Display the usage of this command and exit."), 0, ARG_TYPE_NONE},
    {0, 0, 0, 0, 0, 0}
  };

static struct grub_arg_option
*fnd_short (const struct grub_arg_option *opt, char c)
{
  while (opt->doc)
    {
      if (opt->shortarg == c)
	return (struct grub_arg_option *) opt;
      opt++;
    }
  return 0;
}

static struct grub_arg_option *
find_short (const struct grub_arg_option *options, char c)
{
  struct grub_arg_option *found = 0;

  if (options)
    found = fnd_short (options, c);

  if (! found)
    {
      switch (c)
	{
	case 'h':
	  found = (struct grub_arg_option *) help_options;
	  break;

	case 'u':
	  found = (struct grub_arg_option *) (help_options + 1);
	  break;

	default:
	  break;
	}
    }

  return found;
}

static  struct grub_arg_option
*fnd_long (const struct grub_arg_option *opt, const char *s, int len)
{
  while (opt->doc)
    {
      if (opt->longarg && ! grub_strncmp (opt->longarg, s, len) &&
	  opt->longarg[len] == '\0')
	return (struct grub_arg_option *) opt;
      opt++;
    }
  return 0;
}

static struct grub_arg_option *
find_long (const struct grub_arg_option *options, const char *s, int len)
{
  struct grub_arg_option *found = 0;

  if (options)
    found = fnd_long (options, s, len);

  if (! found)
    found = fnd_long (help_options, s, len);

  return found;
}

static void
show_usage (grub_extcmd_t cmd)
{
  grub_printf ("%s %s %s\n", _("Usage:"), cmd->cmd->name, _(cmd->cmd->summary));
}

static void
showargs (const struct grub_arg_option *opt, int *h_is_used, int *u_is_used)
{
  for (; opt->doc; opt++)
    {
      int spacing = 20;

      if (opt->shortarg && grub_isgraph (opt->shortarg))
	grub_printf ("-%c%c ", opt->shortarg, opt->longarg ? ',':' ');
      else if (opt->shortarg == SHORT_ARG_HELP && ! *h_is_used)
	grub_printf ("-h, ");
      else if (opt->shortarg == SHORT_ARG_USAGE && ! *u_is_used)
	grub_printf ("-u, ");
      else
	grub_printf ("    ");

      if (opt->longarg)
	{
	  grub_printf ("--%s", opt->longarg);
	  spacing -= grub_strlen (opt->longarg) + 2;

	  if (opt->arg)
	    {
	      grub_printf ("=%s", opt->arg);
	      spacing -= grub_strlen (opt->arg) + 1;
	    }
	}

      const char *doc = _(opt->doc);
      for (;;)
	{
	  while (spacing-- > 0)
	    grub_putchar (' ');

	  while (*doc && *doc != '\n')
	    grub_putchar (*doc++);
	  grub_putchar ('\n');

	  if (! *doc)
	    break;
	  doc++;
	  spacing = 4 + 20;
	}

      switch (opt->shortarg)
	{
	case 'h':
	  *h_is_used = 1;
	  break;

	case 'u':
	  *u_is_used = 1;
	  break;

	default:
	  break;
	}
    }
}

void
grub_arg_show_help (grub_extcmd_t cmd)
{
  int h_is_used = 0;
  int u_is_used = 0;

  show_usage (cmd);
  grub_printf ("%s\n\n", _(cmd->cmd->description));
  if (cmd->options)
    showargs (cmd->options, &h_is_used, &u_is_used);
  showargs (help_options, &h_is_used, &u_is_used);
#if 0
  grub_printf ("\nReport bugs to <%s>.\n", PACKAGE_BUGREPORT);
#endif
}

static int
parse_option (grub_extcmd_t cmd, int key, char *arg, struct grub_arg_list *usr)
{
  switch (key)
    {
    case SHORT_ARG_HELP:
      grub_arg_show_help (cmd);
      return -1;

    case SHORT_ARG_USAGE:
      show_usage (cmd);
      return -1;

    default:
      {
	int found = -1;
	int i = 0;
	const struct grub_arg_option *opt = cmd->options;

	while (opt->doc)
	  {
	    if (opt->shortarg && key == opt->shortarg)
	      {
		found = i;
		break;
	      }
	    opt++;
	    i++;
	  }

	if (found == -1)
	  return -1;

	usr[found].set = 1;
	usr[found].arg = arg;
      }
    }

  return 0;
}

static grub_err_t
add_arg (char *s, char ***argl, int *num)
{
  *argl = grub_realloc (*argl, (++(*num)) * sizeof (char *));
  if (! *argl)
    return grub_errno;
  (*argl)[*num - 1] = s;
  return 0;
}

int
grub_arg_parse (grub_extcmd_t cmd, int argc, char **argv,
		struct grub_arg_list *usr, char ***args, int *argnum)
{
  int curarg;
  int arglen;
  int complete = 0;
  char **argl = 0;
  int num = 0;

  for (curarg = 0; curarg < argc; curarg++)
    {
      char *arg = argv[curarg];
      struct grub_arg_option *opt;
      char *option = 0;

      /* No option is used.  */
      if (arg[0] != '-' || grub_strlen (arg) == 1)
	{
	  if (add_arg (arg, &argl, &num) != 0)
	    goto fail;

	  continue;
	}

      /* One or more short options.  */
      if (arg[1] != '-')
	{
	  char *curshort = arg + 1;

	  while (1)
	    {
	      opt = find_short (cmd->options, *curshort);
	      if (! opt)
		{
		  grub_error (GRUB_ERR_BAD_ARGUMENT,
			      "unknown argument `-%c'", *curshort);
		  goto fail;
		}

	      curshort++;

	      /* Parse all arguments here except the last one because
		 it can have an argument value.  */
	      if (*curshort)
		{
		  if (parse_option (cmd, opt->shortarg, 0, usr) || grub_errno)
		    goto fail;
		}
	      else
		{
		  if (opt->type != ARG_TYPE_NONE)
		    {
		      if (curarg + 1 < argc)
			{
			  char *nextarg = argv[curarg + 1];
			  if (!(opt->flags & GRUB_ARG_OPTION_OPTIONAL)
			      || (grub_strlen (nextarg) < 2 || nextarg[0] != '-'))
			    option = argv[++curarg];
			}
		    }
		  break;
		}
	    }

	}
      else /* The argument starts with "--".  */
	{
	  /* If the argument "--" is used just pass the other
	     arguments.  */
	  if (grub_strlen (arg) == 2)
	    {
	      for (curarg++; curarg < argc; curarg++)
		if (add_arg (argv[curarg], &argl, &num) != 0)
		  goto fail;
	      break;
	    }

	  option = grub_strchr (arg, '=');
	  if (option) {
	    arglen = option - arg - 2;
	    option++;
	  } else
	    arglen = grub_strlen (arg) - 2;

	  opt = find_long (cmd->options, arg + 2, arglen);
	  if (! opt)
	    {
	      grub_error (GRUB_ERR_BAD_ARGUMENT, "unknown argument `%s'", arg);
	      goto fail;
	    }
	}

      if (! (opt->type == ARG_TYPE_NONE
	     || (! option && (opt->flags & GRUB_ARG_OPTION_OPTIONAL))))
	{
	  if (! option)
	    {
	      grub_error (GRUB_ERR_BAD_ARGUMENT,
			  "missing mandatory option for `%s'", opt->longarg);
	      goto fail;
	    }

	  switch (opt->type)
	    {
	    case ARG_TYPE_NONE:
	      /* This will never happen.  */
	      break;

	    case ARG_TYPE_STRING:
		  /* No need to do anything.  */
	      break;

	    case ARG_TYPE_INT:
	      {
		char *tail;

		grub_strtoull (option, &tail, 0);
		if (tail == 0 || tail == option || *tail != '\0' || grub_errno)
		  {
		    grub_error (GRUB_ERR_BAD_ARGUMENT,
				"the argument `%s' requires an integer",
				arg);

		    goto fail;
		  }
		break;
	      }

	    case ARG_TYPE_DEVICE:
	    case ARG_TYPE_DIR:
	    case ARG_TYPE_FILE:
	    case ARG_TYPE_PATHNAME:
	      /* XXX: Not implemented.  */
	      break;
	    }
	  if (parse_option (cmd, opt->shortarg, option, usr) || grub_errno)
	    goto fail;
	}
      else
	{
	  if (option)
	    {
	      grub_error (GRUB_ERR_BAD_ARGUMENT,
			  "a value was assigned to the argument `%s' while it "
			  "doesn't require an argument", arg);
	      goto fail;
	    }

	  if (parse_option (cmd, opt->shortarg, 0, usr) || grub_errno)
	    goto fail;
	}
    }

  complete = 1;

  *args = argl;
  *argnum = num;

 fail:
  return complete;
}
