/* 
  Copyright (C) 2000,2003,2004,2005,2006 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright 2007 Sun Microsystems, Inc. All rights reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it would be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  Further, this software is distributed without any warranty that it is
  free of the rightful claim of any third person regarding infringement
  or the like.  Any license provided herein, whether implied or
  otherwise, applies only to this software file.  Patent licenses, if
  any, provided herein do not apply to combinations of this program with
  other software, or any other product whatsoever.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write the Free Software Foundation, Inc., 51
  Franklin Street - Fifth Floor, Boston MA 02110-1301, USA.

  Contact information:  Silicon Graphics, Inc., 1500 Crittenden Lane,
  Mountain View, CA 94043, or:

  http://www.sgi.com

  For further information regarding this notice, see:

  http://oss.sgi.com/projects/GenInfo/NoticeExplan



$Header: /plroot/cmplrs.src/v7.4.5m/.RCS/PL/dwarfdump/RCS/print_sections.c,v 1.69 2006/04/17 00:09:56 davea Exp $ */
#include "globals.h"
#include "dwarf_names.h"
#include "dwconf.h"

#include "print_frames.h"
/*
 * Print line number information:
 * 	filename
 *	new basic-block
 *	[line] [address] <new statement>
 */


static void
  print_pubname_style_entry(Dwarf_Debug dbg,
			    char *line_title,
			    char *name,
			    Dwarf_Unsigned die_off,
			    Dwarf_Unsigned cu_off,
			    Dwarf_Unsigned global_cu_off,
			    Dwarf_Unsigned maxoff);


/* referred in dwarfdump.c */
Dwarf_Die current_cu_die_for_print_frames;

/* If an offset is bad, libdwarf will notice it and
   return an error.
   If it does the error checking in
	print_pubname_style_entry()
   will be useless as we give up here on an error.

   This intended for checking pubnames-style call return
   values (for all the pubnames-style interfaces).

   In at least one gigantic executable die_off turned out wrong.
*/

static void
deal_with_name_offset_err(Dwarf_Debug dbg,
			  char *err_loc,
			  char *name, Dwarf_Unsigned die_off,
			  int nres, Dwarf_Error err)
{
    if (nres == DW_DLV_ERROR) {
	Dwarf_Unsigned myerr = dwarf_errno(err);

	if (myerr == DW_DLE_OFFSET_BAD) {
	    printf("Error: bad offset %s, %s %lld (0x%llx)\n",
		   err_loc,
		   name,
		   (long long) die_off, (unsigned long long) die_off);
	}
	print_error(dbg, err_loc, nres, err);
    }
}

static void
print_source_intro(Dwarf_Die cu_die)
{
    Dwarf_Off off = 0;
    int ores = dwarf_dieoffset(cu_die, &off, &err);

    if (ores == DW_DLV_OK) {
	printf
	    ("Source lines (from CU-DIE at .debug_info offset %llu):\n",
	     (unsigned long long) off);
    } else {
	printf("Source lines (for the CU-DIE at unknown location):\n");
    }
}


extern void
print_line_numbers_this_cu(Dwarf_Debug dbg, Dwarf_Die cu_die)
{
    Dwarf_Signed linecount = 0;
    Dwarf_Line *linebuf = NULL;
    Dwarf_Signed i = 0;
    Dwarf_Addr pc = 0;
    Dwarf_Unsigned lineno = 0;
    Dwarf_Signed column = 0;
    string filename;
    Dwarf_Bool newstatement = 0;
    Dwarf_Bool lineendsequence = 0;
    Dwarf_Bool new_basic_block = 0;
    int lres = 0;
    int sres = 0;
    int ares = 0;
    int lires = 0;
    int cores = 0;

    printf("\n.debug_line: line number info for a single cu\n");
    if (verbose > 1) {
	print_source_intro(cu_die);
	print_one_die(dbg, cu_die, /* print_information= */ 1,
		      /* srcfiles= */ 0, /* cnt= */ 0);

	lres = dwarf_print_lines(cu_die, &err);
	if (lres == DW_DLV_ERROR) {
	    print_error(dbg, "dwarf_srclines details", lres, err);
	}
	return;
    }
    lres = dwarf_srclines(cu_die, &linebuf, &linecount, &err);
    if (lres == DW_DLV_ERROR) {
	print_error(dbg, "dwarf_srclines", lres, err);
    } else if (lres == DW_DLV_NO_ENTRY) {
	/* no line information is included */
    } else {
	print_source_intro(cu_die);
	if (verbose) {
	    print_one_die(dbg, cu_die, /* print_information= */ 1,
			  /* srcfiles= */ 0, /* cnt= */ 0);
	}
	printf
	    ("<source>\t[row,column]\t<pc>\t//<new statement or basic block\n");

	for (i = 0; i < linecount; i++) {
	    Dwarf_Line line = linebuf[i];
	    int nsres;

	    sres = dwarf_linesrc(line, &filename, &err);
	    ares = dwarf_lineaddr(line, &pc, &err);
	    if (sres == DW_DLV_ERROR) {
		print_error(dbg, "dwarf_linesrc", sres, err);
	    }
	    if (sres == DW_DLV_NO_ENTRY) {
		filename = "<unknown>";
	    }
	    if (ares == DW_DLV_ERROR) {
		print_error(dbg, "dwarf_lineaddr", ares, err);
	    }
	    if (ares == DW_DLV_NO_ENTRY) {
		pc = 0;
	    }
	    lires = dwarf_lineno(line, &lineno, &err);
	    if (lires == DW_DLV_ERROR) {
		print_error(dbg, "dwarf_lineno", lires, err);
	    }
	    if (lires == DW_DLV_NO_ENTRY) {
		lineno = -1LL;
	    }
	    cores = dwarf_lineoff(line, &column, &err);
	    if (cores == DW_DLV_ERROR) {
		print_error(dbg, "dwarf_lineoff", cores, err);
	    }
	    if (cores == DW_DLV_NO_ENTRY) {
		column = -1LL;
	    }
	    printf("%s:\t[%3llu,%2lld]\t%#llx", filename, lineno,
		   column, pc);
	    if (sres == DW_DLV_OK)
		dwarf_dealloc(dbg, filename, DW_DLA_STRING);

	    nsres = dwarf_linebeginstatement(line, &newstatement, &err);
	    if (nsres == DW_DLV_OK) {
		if (newstatement) {
		    printf("\t// new statement");
		}
	    } else if (nsres == DW_DLV_ERROR) {
		print_error(dbg, "linebeginstatment failed", nsres,
			    err);
	    }
	    nsres = dwarf_lineblock(line, &new_basic_block, &err);
	    if (nsres == DW_DLV_OK) {
		if (new_basic_block) {
		    printf("\t// new basic block");
		}
	    } else if (nsres == DW_DLV_ERROR) {
		print_error(dbg, "lineblock failed", nsres, err);
	    }
	    nsres = dwarf_lineendsequence(line, &lineendsequence, &err);
	    if (nsres == DW_DLV_OK) {
		if (lineendsequence) {
		    printf("\t// end of text sequence");
		}
	    } else if (nsres == DW_DLV_ERROR) {
		print_error(dbg, "lineblock failed", nsres, err);
	    }
	    printf("\n");

	}
	dwarf_srclines_dealloc(dbg, linebuf, linecount);
    }
}

/*

A strcpy which ensures NUL terminated string
and never overruns the output.

*/
static void
safe_strcpy(char *out, long outlen, char *in, long inlen)
{
    if (inlen >= (outlen - 1)) {
	strncpy(out, in, outlen - 1);
	out[outlen - 1] = 0;
    } else {
	strcpy(out, in);
    }
}

/*
	Returns 1 if a proc with this low_pc found.
	Else returns 0.


*/
static int
get_proc_name(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Addr low_pc,
	      char *proc_name_buf, int proc_name_buf_len)
{
    Dwarf_Signed atcnt = 0;
    Dwarf_Signed i = 0;
    Dwarf_Attribute *atlist = NULL;
    Dwarf_Addr low_pc_die = 0;
    int atres = 0;
    int funcres = 1;
    int funcpcfound = 0;
    int funcnamefound = 1;

    proc_name_buf[0] = 0;	/* always set to something */
    atres = dwarf_attrlist(die, &atlist, &atcnt, &err);
    if (atres == DW_DLV_ERROR) {
	print_error(dbg, "dwarf_attrlist", atres, err);
	return 0;
    }
    if (atres == DW_DLV_NO_ENTRY) {
	return 0;
    }
    for (i = 0; i < atcnt; i++) {
	Dwarf_Half attr;
	int ares;
	string temps;
	int sres;
	int dres;

	if (funcnamefound == 1 && funcpcfound == 1) {
	    /* stop as soon as both found */
	    break;
	}
	ares = dwarf_whatattr(atlist[i], &attr, &err);
	if (ares == DW_DLV_ERROR) {
	    print_error(dbg, "get_proc_name whatattr error", ares, err);
	} else if (ares == DW_DLV_OK) {
	    switch (attr) {
	    case DW_AT_name:
		sres = dwarf_formstring(atlist[i], &temps, &err);
		if (sres == DW_DLV_ERROR) {
		    print_error(dbg,
				"formstring in get_proc_name failed",
				sres, err);
		    /* 50 is safe wrong length since is bigger than the 
		       actual string */
		    safe_strcpy(proc_name_buf, proc_name_buf_len,
				"ERROR in dwarf_formstring!", 50);
		} else if (sres == DW_DLV_NO_ENTRY) {
		    /* 50 is safe wrong length since is bigger than the 
		       actual string */
		    safe_strcpy(proc_name_buf, proc_name_buf_len,
				"NO ENTRY on dwarf_formstring?!", 50);
		} else {
		    long len = (long) strlen(temps);

		    safe_strcpy(proc_name_buf, proc_name_buf_len, temps,
				len);
		}
		funcnamefound = 1;	/* FOUND THE NAME */
		break;
	    case DW_AT_low_pc:
		dres = dwarf_formaddr(atlist[i], &low_pc_die, &err);
		if (dres == DW_DLV_ERROR) {
		    print_error(dbg, "formaddr in get_proc_name failed",
				dres, err);
		    low_pc_die = ~low_pc;
		    /* ensure no match */
		}
		funcpcfound = 1;

		break;
	    default:
		break;
	    }
	}
    }
    for (i = 0; i < atcnt; i++) {
	dwarf_dealloc(dbg, atlist[i], DW_DLA_ATTR);
    }
    dwarf_dealloc(dbg, atlist, DW_DLA_LIST);
    if (funcnamefound == 0 || funcpcfound == 0 || low_pc != low_pc_die) {
	funcres = 0;
    }
    return (funcres);
}

/*
	Modified Depth First Search looking for the procedure:
	a) only looks for children of subprogram.
	b) With subprogram looks at current die *before* looking
	   for a child.
	
	Needed since some languages, including MP Fortran,
	have nested functions.
	Return 0 on failure, 1 on success.
*/
static int
get_nested_proc_name(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Addr low_pc,
		     char *ret_name_buf, int ret_name_buf_len)
{
    char name_buf[BUFSIZ];
    Dwarf_Die curdie = die;
    int die_locally_gotten = 0;
    Dwarf_Die prev_child = 0;
    Dwarf_Die newchild = 0;
    Dwarf_Die newsibling = 0;
    Dwarf_Half tag;
    Dwarf_Error err = 0;
    int chres = DW_DLV_OK;

    ret_name_buf[0] = 0;
    while (chres == DW_DLV_OK) {
	int tres;

	tres = dwarf_tag(curdie, &tag, &err);
	newchild = 0;
	err = 0;
	if (tres == DW_DLV_OK) {
	    int lchres;

	    if (tag == DW_TAG_subprogram) {
		int proc_name_v;

		proc_name_v = get_proc_name(dbg, curdie, low_pc,
					    name_buf, BUFSIZ);
		if (proc_name_v) {
		    /* this is it */
		    safe_strcpy(ret_name_buf, ret_name_buf_len,
				name_buf, (long) strlen(name_buf));
		    if (die_locally_gotten) {
			/* If we got this die from the parent, we do
			   not want to dealloc here! */
			dwarf_dealloc(dbg, curdie, DW_DLA_DIE);
		    }
		    return 1;
		}
		/* check children of subprograms recursively should
		   this really be check children of anything? */

		lchres = dwarf_child(curdie, &newchild, &err);
		if (lchres == DW_DLV_OK) {
		    /* look for inner subprogram */
		    int newprog =
			get_nested_proc_name(dbg, newchild, low_pc,
					     name_buf, BUFSIZ);

		    dwarf_dealloc(dbg, newchild, DW_DLA_DIE);
		    if (newprog) {
			/* Found it.  We could just take this name or
			   we could concatenate names together For now, 
			   just take name */
			if (die_locally_gotten) {
			    /* If we got this die from the parent, we
			       do not want to dealloc here! */
			    dwarf_dealloc(dbg, curdie, DW_DLA_DIE);
			}
			safe_strcpy(ret_name_buf, ret_name_buf_len,
				    name_buf, (long) strlen(name_buf));
			return 1;
		    }
		} else if (lchres == DW_DLV_NO_ENTRY) {
		    /* nothing to do */
		} else {
		    print_error(dbg,
				"get_nested_proc_name dwarf_child() failed ",
				chres, err);
		    if (die_locally_gotten) {
			/* If we got this die from the parent, we do
			   not want to dealloc here! */
			dwarf_dealloc(dbg, curdie, DW_DLA_DIE);
		    }
		    return 0;
		}
	    }			/* end if TAG_subprogram */
	} else {
	    print_error(dbg, "no tag on child read ", tres, err);
	    if (die_locally_gotten) {
		/* If we got this die from the parent, we do not want
		   to dealloc here! */
		dwarf_dealloc(dbg, curdie, DW_DLA_DIE);
	    }
	    return 0;
	}
	/* try next sibling */
	prev_child = curdie;
	chres = dwarf_siblingof(dbg, curdie, &newsibling, &err);
	if (chres == DW_DLV_ERROR) {
	    print_error(dbg, "dwarf_cu_header On Child read ", chres,
			err);
	    if (die_locally_gotten) {
		/* If we got this die from the parent, we do not want
		   to dealloc here! */
		dwarf_dealloc(dbg, curdie, DW_DLA_DIE);
	    }
	    return 0;
	} else if (chres == DW_DLV_NO_ENTRY) {
	    if (die_locally_gotten) {
		/* If we got this die from the parent, we do not want
		   to dealloc here! */
		dwarf_dealloc(dbg, prev_child, DW_DLA_DIE);
	    }
	    return 0;		/* proc name not at this level */
	} else {		/* DW_DLV_OK */
	    curdie = newsibling;
	    if (die_locally_gotten) {
		/* If we got this die from the parent, we do not want
		   to dealloc here! */
		dwarf_dealloc(dbg, prev_child, DW_DLA_DIE);
	    }
	    prev_child = 0;
	    die_locally_gotten = 1;
	}

    }
    if (die_locally_gotten) {
	/* If we got this die from the parent, we do not want to
	   dealloc here! */
	dwarf_dealloc(dbg, curdie, DW_DLA_DIE);
    }
    return 0;
}

/*
  For MP Fortran and possibly other languages, functions 
  nest!  As a result, we must dig thru all functions, 
  not just the top level.


  This remembers the CU die and restarts each search at the start
  of  the current cu.

  
*/
string
get_fde_proc_name(Dwarf_Debug dbg, Dwarf_Addr low_pc)
{
    static char proc_name[BUFSIZ];
    Dwarf_Unsigned cu_header_length;
    Dwarf_Unsigned abbrev_offset;
    Dwarf_Half version_stamp;
    Dwarf_Half address_size;
    Dwarf_Unsigned next_cu_offset = 0;
    int cures = DW_DLV_OK;
    int dres = DW_DLV_OK;
    int chres = DW_DLV_OK;
    int looping = 0;

    if (current_cu_die_for_print_frames == NULL) {
	cures
	    = dwarf_next_cu_header(dbg, &cu_header_length,
				   &version_stamp, &abbrev_offset,
				   &address_size, &next_cu_offset,
				   &err);
	if (cures == DW_DLV_ERROR) {
	    return NULL;
	} else if (cures == DW_DLV_NO_ENTRY) {
	    /* loop thru the list again */
	    current_cu_die_for_print_frames = 0;
	    ++looping;
	} else {		/* DW_DLV_OK */
	    dres = dwarf_siblingof(dbg, NULL,
				   &current_cu_die_for_print_frames,
				   &err);
	    if (dres == DW_DLV_ERROR) {
		return NULL;
	    }
	}
    }
    if (dres == DW_DLV_OK) {
	Dwarf_Die child = 0;

	if (current_cu_die_for_print_frames == 0) {
	    /* no information. Possibly a stripped file */
	    return NULL;
	}
	chres =
	    dwarf_child(current_cu_die_for_print_frames, &child, &err);
	if (chres == DW_DLV_ERROR) {
	    print_error(dbg, "dwarf_cu_header on child read ", chres,
			err);
	} else if (chres == DW_DLV_NO_ENTRY) {
	} else {		/* DW_DLV_OK */
	    int gotname =
		get_nested_proc_name(dbg, child, low_pc, proc_name,
				     BUFSIZ);

	    dwarf_dealloc(dbg, child, DW_DLA_DIE);
	    if (gotname) {
		return (proc_name);
	    }
	    child = 0;
	}
    }
    for (;;) {
	Dwarf_Die ldie;

	cures = dwarf_next_cu_header(dbg, &cu_header_length,
				     &version_stamp, &abbrev_offset,
				     &address_size, &next_cu_offset,
				     &err);

	if (cures != DW_DLV_OK) {
	    break;
	}


	dres = dwarf_siblingof(dbg, NULL, &ldie, &err);

	if (current_cu_die_for_print_frames) {
	    dwarf_dealloc(dbg, current_cu_die_for_print_frames,
			  DW_DLA_DIE);
	}
	current_cu_die_for_print_frames = 0;
	if (dres == DW_DLV_ERROR) {
	    print_error(dbg,
			"dwarf_cu_header Child Read finding proc name for .debug_frame",
			chres, err);
	    continue;
	} else if (dres == DW_DLV_NO_ENTRY) {
	    ++looping;
	    if (looping > 1) {
		print_error(dbg, "looping  on cu headers!", dres, err);
		return NULL;
	    }
	    continue;
	}
	/* DW_DLV_OK */
	current_cu_die_for_print_frames = ldie;
	{
	    int chres;
	    Dwarf_Die child;

	    chres =
		dwarf_child(current_cu_die_for_print_frames, &child,
			    &err);
	    if (chres == DW_DLV_ERROR) {
		print_error(dbg, "dwarf Child Read ", chres, err);
	    } else if (chres == DW_DLV_NO_ENTRY) {

		;		/* do nothing, loop on cu */
	    } else {		/* DW_DLV_OK) */

		int gotname =
		    get_nested_proc_name(dbg, child, low_pc, proc_name,
					 BUFSIZ);

		dwarf_dealloc(dbg, child, DW_DLA_DIE);
		if (gotname) {
		    return (proc_name);
		}
	    }
	}
    }
    return (NULL);
}

/* get all the data in .debug_frame (or .eh_frame). 
 The '3' versions mean print using the dwarf3 new interfaces.
 The non-3 mean use the old interfaces.
 All combinations of requests are possible.
*/
extern void
print_frames(Dwarf_Debug dbg, int print_debug_frame, int print_eh_frame,
	     struct dwconf_s *config_data)
{
    Dwarf_Cie *cie_data = NULL;
    Dwarf_Signed cie_element_count = 0;
    Dwarf_Fde *fde_data = NULL;
    Dwarf_Signed fde_element_count = 0;
    Dwarf_Signed i;
    int fres = 0;
    Dwarf_Half address_size = 0;
    int framed = 0;

    fres = dwarf_get_address_size(dbg, &address_size, &err);
    if (fres != DW_DLV_OK) {
	print_error(dbg, "dwarf_get_address_size", fres, err);
    }
    for (framed = 0; framed < 2; ++framed) {
	char *framename = 0;
	int silent_if_missing = 0;
	int is_eh = 0;

	if (framed == 0) {
	    if (!print_debug_frame) {
		continue;
	    }
	    framename = ".debug_frame";
	    /* 
	     * Big question here is how to print all the info?
	     * Can print the logical matrix, but that is huge,
	     * though could skip lines that don't change.
	     * Either that, or print the instruction statement program
	     * that describes the changes.
	     */
	    fres =
		dwarf_get_fde_list(dbg, &cie_data, &cie_element_count,
				   &fde_data, &fde_element_count, &err);
	} else {
	    if (!print_eh_frame) {
		continue;
	    }
	    is_eh = 1;
	    /* This is gnu g++ exceptions in a .eh_frame section. Which 
	       is just like .debug_frame except that the empty, or
	       'special' CIE_id is 0, not -1 (to distinguish fde from
	       cie). And the augmentation is "eh". As of egcs-1.1.2
	       anyway. A non-zero cie_id is in a fde and is the
	       difference between the fde address and the beginning of
	       the cie it belongs to. This makes sense as this is
	       intended to be referenced at run time, and is part of
	       the running image. For more on augmentation strings, see 
	       libdwarf/dwarf_frame.c.  */

	    /* 
	     * Big question here is how to print all the info?
	     * Can print the logical matrix, but that is huge,
	     * though could skip lines that don't change.
	     * Either that, or print the instruction statement program
	     * that describes the changes.
	     */
	    silent_if_missing = 1;
	    framename = ".eh_frame";
	    fres =
		dwarf_get_fde_list_eh(dbg, &cie_data,
				      &cie_element_count, &fde_data,
				      &fde_element_count, &err);
	}
	if (fres == DW_DLV_ERROR) {
	    printf("\n%s\n", framename);
	    print_error(dbg, "dwarf_get_fde_list", fres, err);
	} else if (fres == DW_DLV_NO_ENTRY) {
	    if (!silent_if_missing)
		printf("\n%s\n", framename);
	    /* no frame information */
	} else {		/* DW_DLV_OK */

	    printf("\n%s\n", framename);
	    printf("\nfde:\n");

	    for (i = 0; i < fde_element_count; i++) {
		print_one_fde(dbg, fde_data[i],
			      i, cie_data, cie_element_count,
			      address_size, is_eh, config_data);
	    }
	    /* 
	       Print the cie set. */
	    if (verbose) {
		printf("\ncie:\n");
		for (i = 0; i < cie_element_count; i++) {
		    print_one_cie(dbg, cie_data[i], i, address_size,
				  config_data);
		}
	    }
	    dwarf_fde_cie_list_dealloc(dbg, cie_data, cie_element_count,
				       fde_data, fde_element_count);
	}
    }
    if (current_cu_die_for_print_frames) {
	dwarf_dealloc(dbg, current_cu_die_for_print_frames, DW_DLA_DIE);
	current_cu_die_for_print_frames = 0;
    }
}




int
  dwarf_get_section_max_offsets(Dwarf_Debug /* dbg */ ,
				Dwarf_Unsigned *	/* debug_info_size 
							 */ ,
				Dwarf_Unsigned *	/* debug_abbrev_size 
							 */
				, Dwarf_Unsigned *	/* debug_line_size 
							 */ ,
				Dwarf_Unsigned *	/* debug_loc_size 
							 */ ,
				Dwarf_Unsigned *
				/* debug_aranges_size */ ,
				Dwarf_Unsigned *
				/* debug_macinfo_size */ ,
				Dwarf_Unsigned *
				/* debug_pubnames_size */ ,
				Dwarf_Unsigned *	/* debug_str_size 
							 */ ,
				Dwarf_Unsigned *	/* debug_frame_size 
							 */
				, Dwarf_Unsigned *	/* debug_ranges_size 
							 */
				, Dwarf_Unsigned *
				/* debug_pubtypes_size */ );

/* The new (April 2005) dwarf_get_section_max_offsets()
   in libdwarf returns all max-offsets, but we only
   want one of those offsets. This function returns 
   the one we want from that set,
   making functions needing this offset as readable as possible.
   (avoiding code duplication).
*/
static Dwarf_Unsigned
get_info_max_offset(Dwarf_Debug dbg)
{
    Dwarf_Unsigned debug_info_size = 0;
    Dwarf_Unsigned debug_abbrev_size = 0;
    Dwarf_Unsigned debug_line_size = 0;
    Dwarf_Unsigned debug_loc_size = 0;
    Dwarf_Unsigned debug_aranges_size = 0;
    Dwarf_Unsigned debug_macinfo_size = 0;
    Dwarf_Unsigned debug_pubnames_size = 0;
    Dwarf_Unsigned debug_str_size = 0;
    Dwarf_Unsigned debug_frame_size = 0;
    Dwarf_Unsigned debug_ranges_size = 0;
    Dwarf_Unsigned debug_pubtypes_size = 0;

    dwarf_get_section_max_offsets(dbg,
				  &debug_info_size,
				  &debug_abbrev_size,
				  &debug_line_size,
				  &debug_loc_size,
				  &debug_aranges_size,
				  &debug_macinfo_size,
				  &debug_pubnames_size,
				  &debug_str_size,
				  &debug_frame_size,
				  &debug_ranges_size,
				  &debug_pubtypes_size);

    return debug_info_size;
}

/* This unifies the code for some error checks to
   avoid code duplication.
*/
static void
check_info_offset_sanity(char *sec,
			 char *field,
			 char *global,
			 Dwarf_Unsigned offset, Dwarf_Unsigned maxoff)
{
    if (maxoff == 0) {
	/* Lets make a heuristic check. */
	if (offset > 0xffffffff) {
	    printf("Warning: section %s %s %s offset 0x%llx "
		   "exceptionally large \n",
		   sec, field, global, (unsigned long long) offset);
	}
    }
    if (offset >= maxoff) {
	printf("Warning: section %s %s %s offset 0x%llx "
	       "larger than max of 0x%llx\n",
	       sec, field, global, (unsigned long long) offset,
	       (unsigned long long) maxoff);
    }
}

/* Unified pubnames style output.
   The error checking here against maxoff may be useless
   (in that libdwarf may return an error if the offset is bad
   and we will not get called here).
   But we leave it in nonetheless as it looks sensible.
   In at least one gigantic executable such offsets turned out wrong.
*/
static void
print_pubname_style_entry(Dwarf_Debug dbg,
			  char *line_title,
			  char *name,
			  Dwarf_Unsigned die_off,
			  Dwarf_Unsigned cu_off,
			  Dwarf_Unsigned global_cu_offset,
			  Dwarf_Unsigned maxoff)
{
    Dwarf_Die die = NULL;
    Dwarf_Die cu_die = NULL;
    Dwarf_Off die_CU_off = 0;
    int dres = 0;
    int ddres = 0;
    int cudres = 0;

    /* get die at die_off */
    dres = dwarf_offdie(dbg, die_off, &die, &err);
    if (dres != DW_DLV_OK)
	print_error(dbg, "dwarf_offdie", dres, err);

    /* get offset of die from its cu-header */
    ddres = dwarf_die_CU_offset(die, &die_CU_off, &err);
    if (ddres != DW_DLV_OK) {
	print_error(dbg, "dwarf_die_CU_offset", ddres, err);
    }

    /* get die at offset cu_off */
    cudres = dwarf_offdie(dbg, cu_off, &cu_die, &err);
    if (cudres != DW_DLV_OK) {
	dwarf_dealloc(dbg, die, DW_DLA_DIE);
	print_error(dbg, "dwarf_offdie", cudres, err);
    }
    printf("%s %-15s die-in-sect %lld, cu-in-sect %lld,"
	   " die-in-cu %lld, cu-header-in-sect %lld",
	   line_title, name, (long long) die_off, (long long) cu_off,
	   /* the cu die offset */
	   (long long) die_CU_off,
	   /* following is absolute offset of the ** beginning of the
	      cu */
	   (long long) (die_off - die_CU_off));

    if ((die_off - die_CU_off) != global_cu_offset) {
	printf(" error: real cuhdr %llu", global_cu_offset);
	exit(1);
    }
    if (verbose) {
	printf(" cuhdr %llu", global_cu_offset);
    }


    dwarf_dealloc(dbg, die, DW_DLA_DIE);
    dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);


    printf("\n");

    check_info_offset_sanity(line_title,
			     "die offset", name, die_off, maxoff);
    check_info_offset_sanity(line_title,
			     "die cu offset", name, die_CU_off, maxoff);
    check_info_offset_sanity(line_title,
			     "cu offset", name,
			     (die_off - die_CU_off), maxoff);

}


/* get all the data in .debug_pubnames */
extern void
print_pubnames(Dwarf_Debug dbg)
{
    Dwarf_Global *globbuf = NULL;
    Dwarf_Signed count = 0;
    Dwarf_Signed i = 0;
    Dwarf_Off die_off = 0;
    Dwarf_Off cu_off = 0;
    char *name = 0;
    int res = 0;

    printf("\n.debug_pubnames\n");
    res = dwarf_get_globals(dbg, &globbuf, &count, &err);
    if (res == DW_DLV_ERROR) {
	print_error(dbg, "dwarf_get_globals", res, err);
    } else if (res == DW_DLV_NO_ENTRY) {
	/* (err == 0 && count == DW_DLV_NOCOUNT) means there are no
	   pubnames.  */
    } else {
	Dwarf_Unsigned maxoff = get_info_max_offset(dbg);

	for (i = 0; i < count; i++) {
	    int nres;
	    int cures3;
	    Dwarf_Off global_cu_off = 0;

	    nres = dwarf_global_name_offsets(globbuf[i],
					     &name, &die_off, &cu_off,
					     &err);
	    deal_with_name_offset_err(dbg, "dwarf_global_name_offsets",
				      name, die_off, nres, err);

	    cures3 = dwarf_global_cu_offset(globbuf[i],
					    &global_cu_off, &err);
	    if (cures3 != DW_DLV_OK) {
		print_error(dbg, "dwarf_global_cu_offset", cures3, err);
	    }

	    print_pubname_style_entry(dbg,
				      "global",
				      name, die_off, cu_off,
				      global_cu_off, maxoff);

	    /* print associated die too? */

	    if (check_pubname_attr) {
		Dwarf_Bool has_attr;
		int ares;
		int dres;
		Dwarf_Die die;

		/* get die at die_off */
		dres = dwarf_offdie(dbg, die_off, &die, &err);
		if (dres != DW_DLV_OK) {
		    print_error(dbg, "dwarf_offdie", dres, err);
		}


		ares =
		    dwarf_hasattr(die, DW_AT_external, &has_attr, &err);
		if (ares == DW_DLV_ERROR) {
		    print_error(dbg, "hassattr on DW_AT_external", ares,
				err);
		}
		pubname_attr_result.checks++;
		if (ares == DW_DLV_OK && has_attr) {
		    /* Should the value of flag be examined? */
		} else {
		    pubname_attr_result.errors++;
		    DWARF_CHECK_ERROR2(name,
				       "pubname does not have DW_AT_external")
		}
		dwarf_dealloc(dbg, die, DW_DLA_DIE);
	    }
	}
	dwarf_globals_dealloc(dbg, globbuf, count);
    }
}				/* print_pubnames() */


struct macro_counts_s {
    long mc_start_file;
    long mc_end_file;
    long mc_define;
    long mc_undef;
    long mc_extension;
    long mc_code_zero;
    long mc_unknown;
};

static void
print_one_macro_entry_detail(long i,
			     char *type,
			     struct Dwarf_Macro_Details_s *mdp)
{
    /* "DW_MACINFO_*: section-offset file-index [line] string\n" */
    if (mdp->dmd_macro) {
	printf("%3ld %s: %6llu %2lld [%4lld] \"%s\" \n",
	       i,
	       type,
	       mdp->dmd_offset,
	       mdp->dmd_fileindex, mdp->dmd_lineno, mdp->dmd_macro);
    } else {
	printf("%3ld %s: %6llu %2lld [%4lld] 0\n",
	       i,
	       type,
	       mdp->dmd_offset, mdp->dmd_fileindex, mdp->dmd_lineno);
    }

}

static void
print_one_macro_entry(long i,
		      struct Dwarf_Macro_Details_s *mdp,
		      struct macro_counts_s *counts)
{

    switch (mdp->dmd_type) {
    case 0:
	counts->mc_code_zero++;
	print_one_macro_entry_detail(i, "DW_MACINFO_type-code-0", mdp);
	break;

    case DW_MACINFO_start_file:
	counts->mc_start_file++;
	print_one_macro_entry_detail(i, "DW_MACINFO_start_file", mdp);
	break;

    case DW_MACINFO_end_file:
	counts->mc_end_file++;
	print_one_macro_entry_detail(i, "DW_MACINFO_end_file  ", mdp);
	break;

    case DW_MACINFO_vendor_ext:
	counts->mc_extension++;
	print_one_macro_entry_detail(i, "DW_MACINFO_vendor_ext", mdp);
	break;

    case DW_MACINFO_define:
	counts->mc_define++;
	print_one_macro_entry_detail(i, "DW_MACINFO_define    ", mdp);
	break;

    case DW_MACINFO_undef:
	counts->mc_undef++;
	print_one_macro_entry_detail(i, "DW_MACINFO_undef     ", mdp);
	break;

    default:
	{
	    char create_type[50];	/* More than large enough. */

	    counts->mc_unknown++;
	    snprintf(create_type, sizeof(create_type),
		     "DW_MACINFO_0x%x", mdp->dmd_type);
	    print_one_macro_entry_detail(i, create_type, mdp);
	}
	break;
    }
}

/* print data in .debug_macinfo */
/* FIXME: should print name of file whose index is in macro data
   here  --  somewhere.
*/
 /*ARGSUSED*/ extern void
print_macinfo(Dwarf_Debug dbg)
{
    Dwarf_Off offset = 0;
    Dwarf_Unsigned max = 0;
    Dwarf_Signed count = 0;
    long group = 0;
    Dwarf_Macro_Details *maclist = NULL;
    int lres = 0;

    printf("\n.debug_macinfo\n");

    while ((lres = dwarf_get_macro_details(dbg, offset,
					   max, &count, &maclist,
					   &err)) == DW_DLV_OK) {
	long i;
	struct macro_counts_s counts;


	memset(&counts, 0, sizeof(counts));

	printf("\n");
	printf("compilation-unit .debug_macinfo # %ld\n", group);
	printf
	    ("num name section-offset file-index [line] \"string\"\n");
	for (i = 0; i < count; i++) {
	    struct Dwarf_Macro_Details_s *mdp = &maclist[i];

	    print_one_macro_entry(i, mdp, &counts);
	}

	if (counts.mc_start_file == 0) {
	    printf
		("DW_MACINFO file count of zero is invalid DWARF2/3\n");
	}
	if (counts.mc_start_file != counts.mc_end_file) {
	    printf("Counts of DW_MACINFO file (%ld) end_file (%ld) "
		   "do not match!.\n",
		   counts.mc_start_file, counts.mc_end_file);
	}
	if (counts.mc_code_zero < 1) {
	    printf("Count of zeros in macro group should be non-zero "
		   "(1 preferred), count is %ld\n",
		   counts.mc_code_zero);
	}
	printf("Macro counts: start file %ld, "
	       "end file %ld, "
	       "define %ld, "
	       "undef %ld "
	       "ext %ld, "
	       "code-zero %ld, "
	       "unknown %ld\n",
	       counts.mc_start_file,
	       counts.mc_end_file,
	       counts.mc_define,
	       counts.mc_undef,
	       counts.mc_extension,
	       counts.mc_code_zero, counts.mc_unknown);


	/* int type= maclist[count - 1].dmd_type; */
	/* ASSERT: type is zero */

	offset = maclist[count - 1].dmd_offset + 1;
	dwarf_dealloc(dbg, maclist, DW_DLA_STRING);
	++group;
    }
    if (lres == DW_DLV_ERROR) {
	print_error(dbg, "dwarf_get_macro_details", lres, err);
    }
}

/* print data in .debug_loc */
extern void
print_locs(Dwarf_Debug dbg)
{
    Dwarf_Unsigned offset = 0;
    Dwarf_Addr hipc_offset = 0;
    Dwarf_Addr lopc_offset = 0;
    Dwarf_Ptr data = 0;
    Dwarf_Unsigned entry_len = 0;
    Dwarf_Unsigned next_entry = 0;
    int lres = 0;

    printf("\n.debug_loc format <o b e l> means "
	   "section-offset begin-addr end-addr length-of-block-entry\n");
    while ((lres = dwarf_get_loclist_entry(dbg, offset,
					   &hipc_offset, &lopc_offset,
					   &data, &entry_len,
					   &next_entry,
					   &err)) == DW_DLV_OK) {
	printf("\t <obel> 0x%08llx 0x%09llx " "0x%08llx " "%8lld\n",
	       (long long) offset, (long long) lopc_offset,
	       (long long) hipc_offset, (long long) entry_len);
	offset = next_entry;
    }
    if (lres == DW_DLV_ERROR) {
	print_error(dbg, "dwarf_get_loclist_entry", lres, err);
    }
}

/* print data in .debug_abbrev */
extern void
print_abbrevs(Dwarf_Debug dbg)
{
    Dwarf_Abbrev ab;
    Dwarf_Unsigned offset = 0;
    Dwarf_Unsigned length = 0;
    Dwarf_Unsigned attr_count = 0;
    Dwarf_Half tag = 0;
    Dwarf_Half attr = 0;
    Dwarf_Signed form = 0;
    Dwarf_Off off = 0;
    Dwarf_Unsigned i = 0;
    string child_name;
    Dwarf_Unsigned abbrev_num = 1;
    Dwarf_Signed child_flag = 0;
    int abres = 0;
    int tres = 0;
    int acres = 0;
    Dwarf_Unsigned abbrev_code = 0;

    printf("\n.debug_abbrev\n");
    while ((abres = dwarf_get_abbrev(dbg, offset, &ab,
				     &length, &attr_count,
				     &err)) == DW_DLV_OK) {

	if (attr_count == 0) {
	    /* Simple innocuous zero : null abbrev entry */
	    if (dense) {
		printf("<%lld><%lld><%lld><%s>\n", abbrev_num, offset, (signed long long)	/* abbrev_code 
												 */ 0,
		       "null .debug_abbrev entry");
	    } else {
		printf("<%4lld><%5lld><code: %2lld> %-20s\n", abbrev_num, offset, (signed long long)	/* abbrev_code 
													 */ 0,
		       "null .debug_abbrev entry");
	    }

	    offset += length;
	    ++abbrev_num;
	    dwarf_dealloc(dbg, ab, DW_DLA_ABBREV);
	    continue;
	}
	tres = dwarf_get_abbrev_tag(ab, &tag, &err);
	if (tres != DW_DLV_OK) {
	    dwarf_dealloc(dbg, ab, DW_DLA_ABBREV);
	    print_error(dbg, "dwarf_get_abbrev_tag", tres, err);
	}
	tres = dwarf_get_abbrev_code(ab, &abbrev_code, &err);
	if (tres != DW_DLV_OK) {
	    dwarf_dealloc(dbg, ab, DW_DLA_ABBREV);
	    print_error(dbg, "dwarf_get_abbrev_code", tres, err);
	}
	if (dense)
	    printf("<%lld><%lld><%lld><%s>", abbrev_num,
		   offset, abbrev_code, get_TAG_name(dbg, tag));
	else
	    printf("<%4lld><%5lld><code: %2lld> %-20s", abbrev_num,
		   offset, abbrev_code, get_TAG_name(dbg, tag));
	++abbrev_num;
	acres = dwarf_get_abbrev_children_flag(ab, &child_flag, &err);
	if (acres == DW_DLV_ERROR) {
	    dwarf_dealloc(dbg, ab, DW_DLA_ABBREV);
	    print_error(dbg, "dwarf_get_abbrev_children_flag", acres,
			err);
	}
	if (acres == DW_DLV_NO_ENTRY) {
	    child_flag = 0;
	}
	child_name = get_children_name(dbg, child_flag);
	if (dense)
	    printf(" %s", child_name);
	else
	    printf("%s\n", child_name);
	/* Abbrev just contains the format of a die, which debug_info
	   then points to with the real data. So here we just print the 
	   given format. */
	for (i = 0; i < attr_count; i++) {
	    int aeres;

	    aeres =
		dwarf_get_abbrev_entry(ab, i, &attr, &form, &off, &err);
	    if (aeres == DW_DLV_ERROR) {
		dwarf_dealloc(dbg, ab, DW_DLA_ABBREV);
		print_error(dbg, "dwarf_get_abbrev_entry", aeres, err);
	    }
	    if (aeres == DW_DLV_NO_ENTRY) {
		attr = -1LL;
		form = -1LL;
	    }
	    if (dense)
		printf(" <%ld>%s<%s>", (unsigned long) off,
		       get_AT_name(dbg, attr),
		       get_FORM_name(dbg, (Dwarf_Half) form));
	    else
		printf("      <%5ld>\t%-28s%s\n",
		       (unsigned long) off, get_AT_name(dbg, attr),
		       get_FORM_name(dbg, (Dwarf_Half) form));
	}
	dwarf_dealloc(dbg, ab, DW_DLA_ABBREV);
	offset += length;
	if (dense)
	    printf("\n");
    }
    if (abres == DW_DLV_ERROR) {
	print_error(dbg, "dwarf_get_abbrev", abres, err);
    }
}

/* print data in .debug_string */
extern void
print_strings(Dwarf_Debug dbg)
{
    Dwarf_Signed length = 0;
    string name;
    Dwarf_Off offset = 0;
    int sres = 0;

    printf("\n.debug_string\n");
    while ((sres = dwarf_get_str(dbg, offset, &name, &length, &err))
	   == DW_DLV_OK) {
	printf("name at offset %lld, length %lld is %s\n",
	       offset, length, name);
	offset += length + 1;
    }
    /* An inability to find the section is not necessarily
       a real error, so do not report error unless we've
       seen a real record. */
    if(sres == DW_DLV_ERROR && offset != 0) {
       print_error(dbg, "dwarf_get_str failure", sres, err);
    }
}

/* get all the data in .debug_aranges */
extern void
print_aranges(Dwarf_Debug dbg)
{
    Dwarf_Signed count = 0;
    Dwarf_Signed i = 0;
    Dwarf_Arange *arange_buf = NULL;
    Dwarf_Addr start = 0;
    Dwarf_Unsigned length = 0;
    Dwarf_Off cu_die_offset = 0;
    Dwarf_Die cu_die = NULL;
    int ares = 0;
    int aires = 0;

    printf("\n.debug_aranges\n");
    ares = dwarf_get_aranges(dbg, &arange_buf, &count, &err);
    if (ares == DW_DLV_ERROR) {
	print_error(dbg, "dwarf_get_aranges", ares, err);
    } else if (ares == DW_DLV_NO_ENTRY) {
	/* no arange is included */
    } else {
	for (i = 0; i < count; i++) {
	    aires = dwarf_get_arange_info(arange_buf[i],
					  &start, &length,
					  &cu_die_offset, &err);
	    if (aires != DW_DLV_OK) {
		print_error(dbg, "dwarf_get_arange_info", aires, err);
	    } else {
		int dres;

		dres = dwarf_offdie(dbg, cu_die_offset, &cu_die, &err);
		if (dres != DW_DLV_OK) {
		    print_error(dbg, "dwarf_offdie", dres, err);
		} else {
		    if (cu_name_flag) {
			Dwarf_Half tag;
			Dwarf_Attribute attrib;
			Dwarf_Half theform;
			int tres;
			int dares;
			int fres;

			tres = dwarf_tag(cu_die, &tag, &err);
			if (tres != DW_DLV_OK) {
			    print_error(dbg, "dwarf_tag in aranges",
					tres, err);
			}
			dares =
			    dwarf_attr(cu_die, DW_AT_name, &attrib,
				       &err);
			if (dares != DW_DLV_OK) {
			    print_error(dbg, "dwarf_attr arange"
					" derived die has no name",
					dres, err);
			}
			fres = dwarf_whatform(attrib, &theform, &err);
			if (fres == DW_DLV_OK) {
			    if (theform == DW_FORM_string
				|| theform == DW_FORM_strp) {
				string temps;
				int sres;

				sres =
				    dwarf_formstring(attrib, &temps,
						     &err);
				if (sres == DW_DLV_OK) {
				    string p = temps;

				    if (cu_name[0] != '/') {
					p = strrchr(temps, '/');
					if (p == NULL) {
					    p = temps;
					} else {
					    p++;
					}
				    }
				    if (!strcmp(cu_name, p)) {
				    } else {
					continue;
				    }
				} else {
				    print_error(dbg,
						"arange: string missing",
						sres, err);
				}
			    }
			} else {
			    print_error(dbg,
					"dwarf_whatform unexpected value",
					fres, err);
			}
			dwarf_dealloc(dbg, attrib, DW_DLA_ATTR);
		    }
		    printf("\narange starts at %llx, "
			   "length of %lld, cu_die_offset = %lld",
			   start, length, cu_die_offset);
		    /* get the offset of the cu header itself in the
		       section */
		    {
			Dwarf_Off off = 0;
			int cures3 =
			    dwarf_get_arange_cu_header_offset(arange_buf
							      [i],
							      &off,
							      &err);

			if (cures3 != DW_DLV_OK) {
			    print_error(dbg, "dwarf_get_cu_hdr_offset",
					cures3, err);
			}
			if (verbose)
			    printf(" cuhdr %llu", off);
		    }
		    printf("\n");
		    print_one_die(dbg, cu_die, (boolean) TRUE,
				  /* srcfiles= */ 0,
				  /* cnt= */ 0);

		    dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
		}
	    }
	    /* print associated die too? */
	    dwarf_dealloc(dbg, arange_buf[i], DW_DLA_ARANGE);
	}
	dwarf_dealloc(dbg, arange_buf, DW_DLA_LIST);
    }
}

/* Get all the data in .debug_static_funcs 
   On error, this allows some dwarf memory leaks.
*/
extern void
print_static_funcs(Dwarf_Debug dbg)
{
    Dwarf_Func *funcbuf = NULL;
    Dwarf_Signed count = 0;
    Dwarf_Signed i = 0;
    Dwarf_Off die_off = 0;
    Dwarf_Off cu_off = 0;
    int gfres = 0;

    printf("\n.debug_static_func\n");
    gfres = dwarf_get_funcs(dbg, &funcbuf, &count, &err);
    if (gfres == DW_DLV_ERROR) {
	print_error(dbg, "dwarf_get_funcs", gfres, err);
    } else if (gfres == DW_DLV_NO_ENTRY) {
	/* no static funcs */
    } else {
	Dwarf_Unsigned maxoff = get_info_max_offset(dbg);

	for (i = 0; i < count; i++) {
	    int fnres;
	    int cures3;
	    Dwarf_Unsigned global_cu_off = 0;
	    char *name = 0;

	    fnres =
		dwarf_func_name_offsets(funcbuf[i], &name, &die_off,
					&cu_off, &err);
	    deal_with_name_offset_err(dbg, "dwarf_func_name_offsets",
				      name, die_off, fnres, err);

	    cures3 = dwarf_func_cu_offset(funcbuf[i],
					  &global_cu_off, &err);
	    if (cures3 != DW_DLV_OK) {
		print_error(dbg, "dwarf_global_cu_offset", cures3, err);
	    }

	    print_pubname_style_entry(dbg,
				      "static-func", name, die_off,
				      cu_off, global_cu_off, maxoff);


	    /* print associated die too? */
	    if (check_pubname_attr) {
		Dwarf_Bool has_attr;
		int ares;
		int dres;
		Dwarf_Die die;

		/* get die at die_off */
		dres = dwarf_offdie(dbg, die_off, &die, &err);
		if (dres != DW_DLV_OK) {
		    print_error(dbg, "dwarf_offdie", dres, err);
		}


		ares =
		    dwarf_hasattr(die, DW_AT_external, &has_attr, &err);
		if (ares == DW_DLV_ERROR) {
		    print_error(dbg, "hassattr on DW_AT_external", ares,
				err);
		}
		pubname_attr_result.checks++;
		if (ares == DW_DLV_OK && has_attr) {
		    /* Should the value of flag be examined? */
		} else {
		    pubname_attr_result.errors++;
		    DWARF_CHECK_ERROR2(name,
				       "pubname does not have DW_AT_external")
		}
		dwarf_dealloc(dbg, die, DW_DLA_DIE);
	    }
	}
	dwarf_funcs_dealloc(dbg, funcbuf, count);
    }
}				/* print_static_funcs() */

/* get all the data in .debug_static_vars */
extern void
print_static_vars(Dwarf_Debug dbg)
{
    Dwarf_Var *varbuf = NULL;
    Dwarf_Signed count = 0;
    Dwarf_Signed i = 0;
    Dwarf_Off die_off = 0;
    Dwarf_Off cu_off = 0;
    char *name = 0;
    int gvres = 0;

    printf("\n.debug_static_vars\n");
    gvres = dwarf_get_vars(dbg, &varbuf, &count, &err);
    if (gvres == DW_DLV_ERROR) {
	print_error(dbg, "dwarf_get_vars", gvres, err);
    } else if (gvres == DW_DLV_NO_ENTRY) {
	/* no static vars */
    } else {
	Dwarf_Unsigned maxoff = get_info_max_offset(dbg);

	for (i = 0; i < count; i++) {
	    int vnres;
	    int cures3;
	    Dwarf_Off global_cu_off = 0;

	    vnres =
		dwarf_var_name_offsets(varbuf[i], &name, &die_off,
				       &cu_off, &err);
	    deal_with_name_offset_err(dbg,
				      "dwarf_var_name_offsets",
				      name, die_off, vnres, err);

	    cures3 = dwarf_var_cu_offset(varbuf[i],
					 &global_cu_off, &err);
	    if (cures3 != DW_DLV_OK) {
		print_error(dbg, "dwarf_global_cu_offset", cures3, err);
	    }

	    print_pubname_style_entry(dbg,
				      "static-var",
				      name, die_off, cu_off,
				      global_cu_off, maxoff);

	    /* print associated die too? */
	}
	dwarf_vars_dealloc(dbg, varbuf, count);
    }
}				/* print_static_vars */

/* get all the data in .debug_types */
extern void
print_types(Dwarf_Debug dbg, enum type_type_e type_type)
{
    Dwarf_Type *typebuf = NULL;
    Dwarf_Signed count = 0;
    Dwarf_Signed i = 0;
    Dwarf_Off die_off = 0;
    Dwarf_Off cu_off = 0;
    char *name = NULL;
    int gtres = 0;

    char *section_name = NULL;
    char *offset_err_name = NULL;
    char *section_open_name = NULL;
    char *print_name_prefix = NULL;
    int (*get_types) (Dwarf_Debug, Dwarf_Type **, Dwarf_Signed *,
		      Dwarf_Error *)
	= 0;
    int (*get_offset) (Dwarf_Type, char **, Dwarf_Off *, Dwarf_Off *,
		       Dwarf_Error *) = NULL;
    int (*get_cu_offset) (Dwarf_Type, Dwarf_Off *, Dwarf_Error *) =
	NULL;
    void (*dealloctype) (Dwarf_Debug, Dwarf_Type *, Dwarf_Signed) =
	NULL;


    if (type_type == DWARF_PUBTYPES) {
	section_name = ".debug_pubtypes";
	offset_err_name = "dwarf_pubtype_name_offsets";
	section_open_name = "dwarf_get_pubtypes";
	print_name_prefix = "pubtype";
	get_types = dwarf_get_pubtypes;
	get_offset = dwarf_pubtype_name_offsets;
	get_cu_offset = dwarf_pubtype_cu_offset;
	dealloctype = dwarf_pubtypes_dealloc;
    } else {
	/* SGI_TYPENAME */
	section_name = ".debug_typenames";
	offset_err_name = "dwarf_type_name_offsets";
	section_open_name = "dwarf_get_types";
	print_name_prefix = "type";
	get_types = dwarf_get_types;
	get_offset = dwarf_type_name_offsets;
	get_cu_offset = dwarf_type_cu_offset;
	dealloctype = dwarf_types_dealloc;
    }



    gtres = get_types(dbg, &typebuf, &count, &err);
    if (gtres == DW_DLV_ERROR) {
	print_error(dbg, section_open_name, gtres, err);
    } else if (gtres == DW_DLV_NO_ENTRY) {
	/* no types */
    } else {
	Dwarf_Unsigned maxoff = get_info_max_offset(dbg);

	/* Before July 2005, the section name was printed
	   unconditionally, now only prints if non-empty section really 
	   exists. */
	printf("\n%s\n", section_name);

	for (i = 0; i < count; i++) {
	    int tnres;
	    int cures3;
	    Dwarf_Off global_cu_off = 0;

	    tnres =
		get_offset(typebuf[i], &name, &die_off, &cu_off, &err);
	    deal_with_name_offset_err(dbg, offset_err_name, name,
				      die_off, tnres, err);

	    cures3 = get_cu_offset(typebuf[i], &global_cu_off, &err);

	    if (cures3 != DW_DLV_OK) {
		print_error(dbg, "dwarf_var_cu_offset", cures3, err);
	    }
	    print_pubname_style_entry(dbg,
				      print_name_prefix,
				      name, die_off, cu_off,
				      global_cu_off, maxoff);

	    /* print associated die too? */
	}
	dealloctype(dbg, typebuf, count);
    }
}				/* print_types() */

/* get all the data in .debug_weaknames */
extern void
print_weaknames(Dwarf_Debug dbg)
{
    Dwarf_Weak *weaknamebuf = NULL;
    Dwarf_Signed count = 0;
    Dwarf_Signed i = 0;
    Dwarf_Off die_off = 0;
    Dwarf_Off cu_off = 0;
    char *name = NULL;
    int wkres = 0;

    printf("\n.debug_weaknames\n");
    wkres = dwarf_get_weaks(dbg, &weaknamebuf, &count, &err);
    if (wkres == DW_DLV_ERROR) {
	print_error(dbg, "dwarf_get_weaks", wkres, err);
    } else if (wkres == DW_DLV_NO_ENTRY) {
	/* no weaknames */
    } else {
	Dwarf_Unsigned maxoff = get_info_max_offset(dbg);

	for (i = 0; i < count; i++) {
	    int tnres;
	    int cures3;

	    Dwarf_Unsigned global_cu_off = 0;

	    tnres = dwarf_weak_name_offsets(weaknamebuf[i],
					    &name, &die_off, &cu_off,
					    &err);
	    deal_with_name_offset_err(dbg,
				      "dwarf_weak_name_offsets",
				      name, die_off, tnres, err);

	    cures3 = dwarf_weak_cu_offset(weaknamebuf[i],
					  &global_cu_off, &err);

	    if (cures3 != DW_DLV_OK) {
		print_error(dbg, "dwarf_weakname_cu_offset",
			    cures3, err);
	    }
	    print_pubname_style_entry(dbg,
				      "weakname",
				      name, die_off, cu_off,
				      global_cu_off, maxoff);

	    /* print associated die too? */
	}
	dwarf_weaks_dealloc(dbg, weaknamebuf, count);
    }
}				/* print_weaknames() */



/*
    decode ULEB
*/
Dwarf_Unsigned
local_dwarf_decode_u_leb128(unsigned char *leb128,
			    unsigned int *leb128_length)
{
    unsigned char byte = 0;
    Dwarf_Unsigned number = 0;
    unsigned int shift = 0;
    unsigned int byte_length = 1;

    byte = *leb128;
    for (;;) {
	number |= (byte & 0x7f) << shift;
	shift += 7;

	if ((byte & 0x80) == 0) {
	    if (leb128_length != NULL)
		*leb128_length = byte_length;
	    return (number);
	}

	byte_length++;
	byte = *(++leb128);
    }
}

#define BITSINBYTE 8
Dwarf_Signed
local_dwarf_decode_s_leb128(unsigned char *leb128,
			    unsigned int *leb128_length)
{
    Dwarf_Signed number = 0;
    Dwarf_Bool sign = 0;
    Dwarf_Signed shift = 0;
    unsigned char byte = *leb128;
    Dwarf_Signed byte_length = 1;

    /* byte_length being the number of bytes of data absorbed so far in
       turning the leb into a Dwarf_Signed. */

    for (;;) {
	sign = byte & 0x40;
	number |= ((Dwarf_Signed) ((byte & 0x7f))) << shift;
	shift += 7;

	if ((byte & 0x80) == 0) {
	    break;
	}
	++leb128;
	byte = *leb128;
	byte_length++;
    }

    if ((shift < sizeof(Dwarf_Signed) * BITSINBYTE) && sign) {
	number |= -((Dwarf_Signed) 1 << shift);
    }

    if (leb128_length != NULL)
	*leb128_length = byte_length;
    return (number);
}


/* Dumping a dwarf-expression as a byte stream. */
void
dump_block(char *prefix, char *data, Dwarf_Signed len)
{
    char *end_data = data + len;
    char *cur = data;
    int i = 0;

    printf("%s", prefix);
    for (; cur < end_data; ++cur, ++i) {
	if (i > 0 && i % 4 == 0)
	    printf(" ");
	printf("%02x", 0xff & *cur);

    }
}
