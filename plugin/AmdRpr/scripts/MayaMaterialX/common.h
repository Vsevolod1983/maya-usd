#pragma once

#include <maya/MFnAttribute.h>

void MAKE_INPUT(MFnAtrribute& attr)
{
	CHECK_MSTATUS(attr.setKeyable(true));
	CHECK_MSTATUS(attr.setStorable(true));
	CHECK_MSTATUS(attr.setReadable(false));
	CHECK_MSTATUS(attr.setWritable(true));
}

void MAKE_INPUT_CONST(MFnAtrribute& attr)
{
	CHECK_MSTATUS(attr.setKeyable(false));
	CHECK_MSTATUS(attr.setStorable(true));
	CHECK_MSTATUS(attr.setReadable(false));
	CHECK_MSTATUS(attr.setWritable(true));
	CHECK_MSTATUS(attr.setConnectable(false));
}

void MAKE_OUTPUT(MFnAtrribute& attr)
{
	CHECK_MSTATUS(attr.setKeyable(false));
	CHECK_MSTATUS(attr.setStorable(false));
	CHECK_MSTATUS(attr.setReadable(true));
	CHECK_MSTATUS(attr.setWritable(false));
}
