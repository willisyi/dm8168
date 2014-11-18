/*
 *  ======== genaddrinfoscript.xs ========
 *  Javascript that generates a unix shell script
 *  containining address info from the config.bld
 *  This file must be loaded from config.bld
 *  using xdc.loadCapsule();
 */



function Value2HexString(Value)
{
    return ("0x" + java.lang.Long.toHexString(Value));
}

function IntType(Value)
{
    return (Value>>>0);
}

function Size2MB(Size)
{
    return (IntType(Size)/MB);
}

function GetQuotedString(Value)
{
    return ("\""+Value+"\"");
}

function GetShellExportString(EnvVariable,Value)
{
    return ("export " + EnvVariable+"="+GetQuotedString(Value));
}

function GenAddrFile()
{
    print ("Generation of Shell script in progress...");
    /* Create the value of the environmental variables to be set */
    var RDK_LINUX_MEM_STR           = Size2MB(LINUX_SIZE) + "M";
    var NOTIFYK_VPSSM3_SVA_ADDR_STR = Value2HexString(NOTIFY_SHARED_ADDR);
    var REMOTE_DEBUG_ADDR_STR       = Value2HexString(REMOTE_DEBUG_ADDR);
    var HDVPSS_SHARED_MEM_STR       = Value2HexString(HDVPSS_SHARED_ADDR);
    var HDVPSS_SHARED_SIZE_STR       = Value2HexString(HDVPSS_SHARED_SIZE);

    /* Create the env.sh file */
    var File = xdc.useModule('xdc.services.io.File');
    var fd = File.open("./env.sh","w");
    fd.writeLine("#!/bin/sh                                                                    ");
    fd.writeLine("#RDK_LINUX_MEM:                                                              ");
    fd.writeLine("#The amount of memory allocated to linux.                                    ");
    fd.writeLine("#The kernel bootargs mem= parameter should match this value.                 ");
    fd.writeLine(GetShellExportString("RDK_LINUX_MEM",RDK_LINUX_MEM_STR));
    fd.writeLine("#The start address of kernel NOTIFY_MEM                                      ");
    fd.writeLine("");
    fd.writeLine("#The kernel bootargs notifyk.vpssm3_sva= parameter should match this value.  ");
    fd.writeLine(GetShellExportString("NOTIFYK_VPSSM3_SVA_ADDR",NOTIFYK_VPSSM3_SVA_ADDR_STR));
    fd.writeLine("");
    fd.writeLine("#The start address of REMOTE_DEBUG_ADDR section                              ");
    fd.writeLine("#The address of REMOTE_DEBUG_MEM in the slave executables should match this  ");
    fd.writeLine(GetShellExportString("REMOTE_DEBUG_ADDR",REMOTE_DEBUG_ADDR_STR));
    fd.writeLine("");
    fd.writeLine("#The start address of HDVPSS_SHARED_MEM section                              ");
    fd.writeLine("#The address of HDVPSS_SHARED_MEM in the slave executables should match this ");
    fd.writeLine(GetShellExportString("HDVPSS_SHARED_MEM",HDVPSS_SHARED_MEM_STR));
    fd.writeLine("");
    fd.writeLine("#The size of HDVPSS_SHARED_MEM section                              ");
    fd.writeLine(GetShellExportString("HDVPSS_SHARED_SIZE",HDVPSS_SHARED_SIZE_STR));
    fd.writeLine("");

    fd.close();
    print ("Generation of Shell script in completed...");
}
