#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for backend/commands
#
# IDENTIFICATION
#    src/backend/commands/Makefile
#
#-------------------------------------------------------------------------

subdir = src/backend/commands
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

OBJS = \
	aggregatecmds.o \
	alter.o \
	amcmds.o \
	analyze.o \
	async.o \
	cluster.o \
	collationcmds.o \
	comment.o \
	constraint.o \
	conversioncmds.o \
	copy.o \
	copyfrom.o \
	copyfromparse.o \
	copyto.o \
	createas.o \
	createkek.o \
	createdek.o \
	dbcommands.o \
	define.o \
	discard.o \
	dropcmds.o \
	dropkek.o \
	dropdek.o \
	encrypted_column.o \
	event_trigger.o \
	explain.o \
	extension.o \
	foreigncmds.o \
	functioncmds.o \
	indexcmds.o \
	lockcmds.o \
	matview.o \
	opclasscmds.o \
	operatorcmds.o \
	policy.o \
	portalcmds.o \
	prepare.o \
	proclang.o \
	publicationcmds.o \
	schemacmds.o \
	seclabel.o \
	sequence.o \
	statscmds.o \
	subscriptioncmds.o \
	resgroupcmds.o \
	tablecmds.o \
	tecc_tabledef.o \
	tablespace.o \
	trigger.o \
	tsearchcmds.o \
	typecmds.o \
	user.o \
	vacuum.o \
	variable.o \
	view.o \
	packagecmds.o \
	flashback.o \
	ora_trigger_order.o \
	ora_trigger_stack.o \
	objecttypecmds.o

include $(top_srcdir)/src/backend/common.mk

# TECC后端命令文件头文件依赖
TECC_HEADERS = $(top_srcdir)/src/include/utils/cipherdatatype.h \
               $(top_srcdir)/src/include/commands/keycmd.h \
               $(top_srcdir)/src/include/commands/createkek.h \
               $(top_srcdir)/src/include/commands/createdek.h \
               $(top_srcdir)/src/include/commands/dropkek.h \
               $(top_srcdir)/src/include/commands/dropdek.h \
               $(top_srcdir)/src/include/commands/encrypted_column.h \
               $(top_srcdir)/src/include/commands/tecc_tabledef.h

createkek.o createdek.o dropkek.o dropdek.o encrypted_column.o tecc_tabledef.o: $(TECC_HEADERS)
