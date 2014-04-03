## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src
ACLOCAL_AMFLAGS = -I m4

# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(sickycatdocdir)

install: install-recursive
	@echo "Installing support-files"
	@mkdir -p $(prefix)/etc --mode=0755
	@mkdir -p $(prefix)/log --mode=0755
	@mkdir -p $(prefix)/script --mode=0755
	@mkdir -p $(prefix)/run --mode=0755
	@cp --backup $(srcdir)/preinstall-files/bsp-server.conf.in $(prefix)/etc/bsp-server.conf
	@cp --backup $(srcdir)/preinstall-files/main.lua.in $(prefix)/script/main.lua
	@echo ""
	@echo "************************************************************"
	@echo "*             THANK YOU FOR CHOOSING BS.PLAY               *"
	@echo "*==========================================================*"
	@echo "* This program is free software; you can redistribute it   *"
	@echo "* and/or modify it under the terms of the GNU General      *"
	@echo "* Public License as published by the Free Software         *"
	@echo "* Foundation; either version 2 of the License, or (at your *"
	@echo "* option) any later version.                               *"
	@echo "*                                                          *"
	@echo "* This program is distributed in the hope that it will be  *"
	@echo "* useful, but WITHOUT ANY WARRANTY; without even the       *"
	@echo "* implied warranty of MERCHANTABILITY or FITNESS FOR A     *"
	@echo "* PARTICULAR PURPOSE. See the GNU Library General Public   *"
	@echo "* License for more details.                                *"
	@echo "*                                                          *"
	@echo "* You should have received a copy of the GNU General       *"
	@echo "* Public License along with this program; if not, write to *"
	@echo "* the Free Software Foundation, Inc., 51 Franklin Street,  *"
	@echo "* Fifth Floor Boston, MA 02110-1301,  USA                  *"
	@echo "*==========================================================*"
	@echo "* BS.Play official site:                                   *"
	@echo "*          http://bsp.bsgroup.org/                         *"
	@echo "* BS.Group:                                                *"
	@echo "*          http://www.bsgroup.org/                         *"
	@echo "* Author:                                                  *"
	@echo "*          Dr.NP <np@bsgroup.org>                          *"
	@echo "************************************************************"
	@echo ""
