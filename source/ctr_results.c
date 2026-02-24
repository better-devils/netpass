/**
 * NetPass
 * Copyright (C) 2025 Sorunome
 *               2026 Silentium
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// converted from https://github.com/nikita-petko/ctr-result-parser
// Licensed under Apache 2.0

#include "ctr_results.h"
#include <stdio.h>

static const char* const level_to_string[] = {
	"Success",
	"Info",
	"Status",
	"Temporary",
	"Permanent",
	"Usage",
	"Reinit",
	"Reset",
	"Fatal",
};

static const char* const level_description[] = {
	"Success. There is no additional information available.",
	"Success. There is additional information available.",
	"Expected failure.",
	"Temporary failure. You can try again immediately with the same argument. Succeeds if the number of attempts is relatively low.",
	"Common error. You cannot try again.",
	"Programming error. Indicates a programming error that always occurs with the argument values that were specified, regardless of the particular internal state of the library.",
	"Error that requires the module to be reinitialized.",
	"Unexpected error. The best course of action is to transtion to the reset sequence. Reinitializing the module is not likely to lead to recovery, but it may be possible to recover by resetting.",
	"Fatal system level error. Software recovery not possible. Jump to support center guide sequence.",
};

static const char* const summary_to_string[] = {
	"Success",
	"Nothing Happened",
	"Would Block",
	"Out Of Resource",
	"Not Found",
	"Invalid State",
	"Not Supported",
	"Invalid Argument",
	"Wrong Argument",
	"Cancelled",
	"Status Changed",
	"Internal",
};

static const char* const summary_description[] = {
	"Succeeded.",
	"Nothing happens.",
	"The process may be blocked.",
	"The resources needed for the process could not be allocated.",
	"The object does not exist.",
	"The requested process cannot be executed with the current internal state.",
	"Not currently supported by the SDK.",
	"The argument is invalid.",
	"A parameter other than the argument is invalid.",
	"The process was cancelled.",
	"The status changed. (Example: Internal status changed while the process was running.)",
	"Error that is used within the library.",
};

static const char* const module_to_string[] = {
	"cmn",
	"kern",
	"util",
	"fs srv",
	"ldr srv",
	"tcb",
	"os",
	"dbg",
	"dmnt",
	"pdn",
	"gx",
	"i2c",
	"gpio",
	"dd",
	"codec",
	"spi",
	"pxi",
	"fs",
	"di",
	"hid",
	"cam",
	"pi",
	"pm",
	"pm low",
	"fsi",
	"srv",
	"ndm",
	"nwm",
	"soc",
	"ldr",
	"acc",
	"romfs",
	"am",
	"hio",
	"upd",
	"mic",
	"fnd",
	"mp",
	"mpwl",
	"ac",
	"http",
	"dsp",
	"snd",
	"dlp",
	"hio low",
	"csnd",
	"ssl",
	"am low",
	"nex",
	"frd",
	"rdt",
	"app",
	"nim",
	"ptm",
	"midi",
	"mc",
	"swc",
	"fatfs",
	"ngc",
	"card",
	"card nor",
	"sdmc",
	"boss",
	"dbm",
	"cfg",
	"ps",
	"cec",
	"ir",
	"uds",
	"pl",
	"cup",
	"gyro",
	"mcu",
	"ns",
	"news",
	"ro",
	"gd",
	"card spi",
	"ec",
	"web brs",
	"test",
	"enc",
	"pia",
	"act",
	"vctl",
	"olv",
	"neia",
	"npns",

	"reserved0",
	"reserved1",

	"avd",
	"l2b",
	"mvd",
	"nfc",
	"uart",
	"spm",
	"qtm",
	"nfp",
	"npt",
};

static const char* const module_description[] = {
	"Common",
	"Kernel",
	"Util",
	"File Server",
	"Loader Server",
	"Tcb",
	"OS",
	"Debug",
	"Debug Monitor",
	"Power Domain",
	"Graphics Driver",
	"Inter-Integrated Circuit Driver",
	"General Purpose Input/Output",
	"Daemon Driver",
	"Codec Driver",
	"Serial Peripheral Interface Driver",
	"Process eXecution Interface",
	"File System",
	"Device Interface",
	"Human Interface Device",
	"Camera",
	"Process Interface",
	"Process Manager",
	"Process Manager Low",
	"File System Interface",
	"Services",
	"Network Daemon Manager",
	"Network Wireless Manager",
	"Socket",
	"Loader",
	"Account Management",
	"Read-Only File System",
	"Application Manager",
	"Host IO",
	"Updater",
	"Microphone",
	"Foundation",
	"Multiplayer",
	"Multiplayer Wireless",
	"Automatic Connection",
	"Hypertext Transfer Protocol",
	"Digital Signal Processor",
	"Sound",
	"Download Play",
	"Host IO Low",
	"Codec Sound",
	"Secure Sockets Layer",
	"Application Manager Low",
	"NEX Game Networking",
	"Friends",
	"Reliable Data Transport",
	"Applet",
	"Nintendo Internet Module",
	"Power/Time Management",
	"Musical Instrument Digital Interface",
	"Media Control",
	"Software Controller",
	"File Allocation Table File System",
	"Name Generation Control",
	"Card",
	"Card NOR",
	"Secure Digital Memory Card",
	"Background Operating System Service",
	"Database Manager",
	"Configuration",
	"Process Signatures",
	"Chance Encounter Communication",
	"Infrared",
	"User Datagram Service",
	"Play Log",
	"Card Update",
	"Gyroscope",
	"Microcontroller",
	"Network System", // Another IS system module.
	"News",
	"Read-Only",
	"Graphics Data Flow",
	"Card Serial Peripheral Interface",
	"Nintendo E-Shop",
	"Web Browser",
	"Test",
	"Encryption",
	"PIA",
	"Account",
	"Version Control",
	"OLV",
	"NEIA",
	"NPNS",

	"Reserved0",
	"Reserved1",

	"AVD",
	"Line To Block",
	"Movie Decoder",
	"Near Field Communication",
	"Universal Asynchronous Receiver/Transmitter",
	"SPM",
	"Quartet",
	"Near Field Proximity",
	"NPT",
};

static const char* const description_to_string[] = {
	"Invalid Selection",
	"Too Large",
	"Not Authorized",
	"Already Done",
	"Invalid Size",
	"Invalid Enum Value",
	"Invalid Combination",
	"No Data",
	"Busy",
	"Misaligned Address",
	"Misaligned Size",
	"Out Of Memory",
	"Not Implemented",
	"Invalid Address",
	"Invalid Pointer",
	"Invalid Handle",
	"Not Initialized",
	"Already Initialized",
	"Not Found",
	"Cancel Requested",
	"Already Exists",
	"Out Of Range",
	"Timeout",
	"Invalid Result Value",
};

static const char* const description_description[] = {
	"An invalid value was specified when a specifiable value is discrete.",
	"The value is too large.",
	"Unauthorized operation.",
	"The internal status has already been specified.",
	"Invalid size.",
	"The value is outside the range for enum values.",
	"Invalid parameter combination.",
	"No data.",
	"Could not be run because another process was already being performed.",
	"Invalid address alignment.",
	"Invalid size alignment.",
	"Insufficient memory.",
	"Not yet implemented.",
	"Invalid address.",
	"Invalid pointer.",
	"Invalid handle.",
	"Not initialized.",
	"Already initialized.",
	"The object does not exist.",
	"Request canceled.",
	"The object already exists.",
	"The value is outside of the defined range.",
	"The process timed out.",
	"These values are not used.",
};

static const char* get_desc_str_ac(int desc) {
	switch (desc) {
		case 50: return "WanConnected";
		case 51: return "LanConnected";
		case 52: return "UnnecessaryHotspotLogout";
		case 70: return "Processing";
		case 100: return "FailedStartup";
		case 101: return "FailedConnectAp";
		case 102: return "FailedDhcp";
		case 103: return "ConflictIpAddress";
		case 104: return "InvalidKeyValue";
		case 105: return "UnsupportAuthAlgorithm";
		case 106: return "DenyUsbAp";
		case 150: return "InvalidDns";
		case 151: return "InvalidProxy";
		case 152: return "FailedConnTest";
		case 200: return "UnsupportHotspot";
		case 201: return "FailedHotspotAuthentification";
		case 202: return "FailedHotspotConnTest";
		case 203: return "UnsupportPlace";
		case 204: return "FailedHotspotLogout";
		case 205: return "AlreadyConnectUnsupportAp";
		case 300: return "FailedScan";
		case 301: return "AlreadyConnecting";
		case 302: return "NotConnecting";
		case 303: return "AlreadyExclusive";
		case 304: return "NotExclusive";
		case 305: return "InvalidLocation";
		case 900: return "NotAgreeEula";
		case 901: return "WifiOff";
		case 902: return "BrokenNand";
		case 903: return "BrokenWireless";
	}
	return NULL;
}

static const char* get_desc_desc_ac(int desc) {
	switch (desc) {
		case 50: return "WAN connection established";
		case 51: return "LAN connection established";
		case 52: return "Hotspot authentication logout is unnecessary";
		case 70: return "Connecting";
		case 100: return "Wireless device initialization failed";
		case 101: return "Failed to connect to the access point";
		case 102: return "Failed to obtain IP address";
		case 103: return "IP address conflict";
		case 104: return "Incorrect encryption key";
		case 105: return "Unsupported authentication mode";
		case 106: return "A connection to a Nintendo Wi-Fi USB Connector was denied";
		case 150: return "Failed to resolve name";
		case 151: return "Failed to connect to the proxy server";
		case 152: return "Failed to connect to the HTTP server";
		case 200: return "Connection from unsupported access point";
		case 201: return "Failed hotspot authentication";
		case 202: return "Failed to connect to to HTTP server after hotspot authentication";
		case 203: return "Application cannot use the Internet from present physical location";
		case 204: return "Failed hotspot authentication logout";
		case 205: return "Already connected to an unsupported access point";
		case 300: return "Scan failed";
		case 301: return "Already connecting";
		case 302: return "Not connected";
		case 303: return "Already exclusive";
		case 304: return "Not exclusive";
		case 305: return "Invalid location";
		case 900: return "The user has not agreed to the EULA";
		case 901: return "Disabled";
		case 902: return "The NAND device is damaged or malfunctioning";
		case 903: return "The wireless device is damaged or malfunctioning";
	}
	return NULL;
}

static const char* get_desc_str_am(int desc) {
	const char* const map[] = {
		"InvalidCiaFormat",
		"InvalidImportPipeState",
		"InvalidCiaReaderState",
		"InvalidImportState",
		"InvalidUpdateState",
		"TooLargeFileSize",
		"InvalidContent",
		"UnsupportedPipeOperation",
		"ImportPipealreadyOpen",
		"InvalidPipe",
		"InvalidFirmFormat",
		"FailedToVerifyFirmware",
		"FirmwareHashUnmatched",
		"InvalidFirmwareProgramId",
		"ImportNotBegun",
		"UnexpectedEsError",
		"NoSuchDbEntry",
		"ImportIncompleted",
		"CiaMetaNotFound",
		"TooManySessions",
		"InvalidTmdInCia",
		"InvalidSystemMenuDataSize",
		"FailedToAddTicketDb",
		"FailedToGetTicketDb",
		"InvalidTicketVersion",
		"NoEsCertifcatesFound",
		"TooManyCertifcates",
		"NotFoundJumpId",
		"ImportContextNotFound",
		"MediaNotSupported",
		"InvalidArgument",
		"InvalidInputStream",
		"InvalidDbFormat",
		"InternalDataCorrupted",
		"FilesystemError",
		"EsError",
		"VerificationFailed",
		"NotResumable",
		"InvalidVersion",
		"NotSupported",
		"UnknownEsError",
		"InvalidSaveDataSize",
		"DatabaseNotFound",
		"ProgramNotDeletable",
		"InvalidDatabaseState",
		"UniqueIdMatch",
		"UniqueIdMismatch",
		"HashCheckFailed",
		"DecryptionFailed",
		"BkpWriteFailed",
		"BkpReadFailed",
		"InvalidBkpFormat",
		"NoEnoughSpace",
		"TicketDbOverflow",
		"InvalidTicketTitleId",
		"MainContentNotFound",
		"NoRight",
		"InvalidTicketSize",
		"ContentNotFound",
		"InvalidProgramId",
		"UnsupportedNumContents",
		"UndeletableContents",
		"BufferNotEnough",
		"TicketNotFound",
		"InvalidTitleCombination",
		"ContextVerificatonFailed",
		"UnsupportedContextVersion",
	};
	const char* const map2[] = {
		"EsOk",
		"EsFail",
		"EsInvalid",
		"EsStorage",
		"EsStorageSize",
		"EsCrypto",
		"EsVerification",
		"EsDeviceIdMismatch",
		"EsIssuerNotFound",
		"EsIncorrectSigType",
		"EsIncorrectPubkeyType",
		"EsIncorrectTicketVersion",
		"EsIncorrectTmdVersion",
		"EsNoRight",
		"EsAlignment",
	};
	if (desc >= 1 && desc <= 67) return map[desc - 1];
	if (desc >= 100 && desc <= 114) return map2[desc - 100];
	return NULL;
}

static const char* get_desc_desc_am(int desc) {
	const char* const map[] = {
		"Invalid CIA format",
		"Invalid import pipe state",
		"Invalid CIA reader state",
		"Invalid import state",
		"Invalid update state",
		"Too large file size",
		"Invalid content",
		"Unsupported pipe operation",
		"Import pipe already open",
		"Invalid pipe",
		"Invalid firm format",
		"Failed to verify firmware",
		"Firmware hash unmatched",
		"Invalid firmware program ID",
		"Import not begun",
		"Unexpected ES error",
		"No such DB entry",
		"Import incompleted",
		"CIA meta not found",
		"Too many sessions",
		"Invalid TMD in CIA",
		"Invalid system menu data size",
		"Failed to add ticket DB",
		"Failed to get ticket DB",
		"Invalid ticket version",
		"No ES certifcates found",
		"Too many certifcates",
		"Not found jump ID",
		"Import context not found",
		"Media not supported",
		"Invalid argument",
		"Invalid input stream",
		"Invalid DB format",
		"Internal data corrupted",
		"Filesystem error",
		"ES error",
		"Verification failed",
		"Not resumable",
		"Invalid version",
		"Not supported",
		"Unknown ES error",
		"Invalid save data size",
		"Database not found",
		"Program not deletable",
		"Invalid database state",
		"Unique ID match",
		"Unique ID mismatch",
		"Hash check failed",
		"Decryption failed",
		"BKP write failed",
		"BKP read failed",
		"Invalid BKP format",
		"No enough space",
		"Ticket DB overflow",
		"Invalid ticket title ID",
		"Main content not found",
		"No right",
		"Invalid ticket size",
		"Content not found",
		"Invalid program ID",
		"Unsupported num contents",
		"Undeletable contents",
		"Buffer not enough",
		"Ticket not found",
		"Invalid title combination",
		"Context verificaton failed",
		"Unsupported context version",
	};
	const char* const map2[] = {
		"ES OK",
		"ES fail",
		"ES invalid",
		"ES storage",
		"ES storage size",
		"ES crypto",
		"ES verification",
		"ES device ID mismatch",
		"ES issuer not found",
		"ES incorrect sig type",
		"ES incorrect pubkey type",
		"ES incorrect ticket version",
		"ES incorrect TMD version",
		"ES no right",
		"ES alignment",
	};
	if (desc >= 1 && desc <= 67) return map[desc - 1];
	if (desc >= 100 && desc <= 114) return map2[desc - 100];
	return NULL;
}

static const char* get_desc_str_applet(int desc) {
	const char* const map[] = {
		"AppletNoAreaToRegister",
		"AppletParameterBufferNotEmtpy",
		"AppletCallerNotFound",
		"AppletNotAllowed",
		"AppletDifferentMode",
		"AppletDifferentVersion",
		"AppletShutdownProcessing",
		"AppletTransitionBusy",
		"AppletVersionMustLaunchDirectly",
		"InvalidQuery",
		"NoApplications",
		"TooOldSystemUpdater",
	};
	if (desc >= 1 && desc <= 12) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_applet(int desc) {
	const char* const map[] = {
		"There is no space in the table used for registration.",
		"The parameter region is not empty.",
		"The process that called the applet does not exist.",
		"Access is denied.",
		"Different mode.",
		"Different version.",
		"Shutting down.",
		"Something else is in the middle of a transition.",
		"A different version that must be started directly without using menus.",
		"Invalid query.",
		"No applications.",
		"The system update is too old.",
	};
	if (desc >= 1 && desc <= 12) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_boss(int desc) {
	const char* const map[] = {
		"InvalidPolicy",
		"InvalidAction",
		"InvalidOption",
		"InvalidAppIdList",
		"InvalidTaskIdList",
		"InvalidStepIdList",
		"InvalidNsDataIdList",
		"InvalidTaskStatus",
		"InvalidPropertyValue",
		"InvalidNewArrivalEvent",
		"InvalidNewArrivalFlag",
		"InvalidOptOutFlag",
		"InvalidTaskError",
		"InvalidNsDataValue",
		"InvalidNsDataInfo",
		"InvalidNsDataReadFlag",
		"InvalidNsDataTime",
		"InvalidNextExecuteTime",
		"HttpRequestHeaderPointerNull",
		"InvalidPolicyListAvailability",
		"InvalidTestModeAvailability",
		"InvalidConfigParentalControl",
		"InvalidConfigEulaAgreement",
		"InvalidPolicyListEnvId",
		"InvalidPolicyListUrl",
		"InvalidTaskId",
		"InvalidTaskStep",
		"InvalidPropertyType",
		"InvalidUrl",
		"InvalidFilePath",
		"InvalidTaskPriority",
		"InvalidTaskTargetDuration",
		"ActionCodeOutOfRange",
		"InvalidNsDataSeekPosition",
		"InvalidMaxHttpRequestHeader",
		"InvalidMaxClientCert",
		"InvalidMaxRootCa",
		"SchedulingPolicyOutOfRange",
		"ApInfoTypeOutOfRange",
		"TaskPermissionOutOfRange",
		"WaitFinishTimeout",
		"WaitFinishTaskNotDone",
		"IpcNotSessionInitialized",
		"IpcPropertySizeError",
		"IpcTooManyRequests",
		"AlreadyInitialized",
		"OutOfMemory",
		"InvalidNumberOfNsd",
		"NsDataInvalidFormat",
		"ApliNotExist",
		"TaskNotExist",
		"TaskStepNotExist",
		"ApliIdAlreadyExist",
		"TaskIdAlreadyExist",
		"TaskStepAlreadyExist",
		"InvalidSequence",
		"DatabaseFull",
		"CantUnregisterTask",
		"TaskNotRegistered",
		"InvalidFilehandle",
		"InvalidTaskSchedulingPolicy",
		"InvalidHttpRequestHeader",
		"InvalidHeadtype",
		"StorageAccesspermission",
		"StorageInsufficiency",
		"InvalidAppidStorageNotfound",
		"NsdataNotfound",
		"InvalidNsdataGetheadSize",
		"NsdataListSizeShortage",
		"NsdataListUpdated",
		"NotConnectApWithLocation",
		"NotConnectNetwork",
		"InvalidFriendcode",
		"FileAccess",
		"TaskAlreadyPaused",
		"TaskAlreadyResumed",
		"Unexpect",
	};
	const char* const map2[] = {
		"InvalidStorageParameter",
		"CfginfotypeOutOfRange",
		"InvalidMaxHttpQuery",
		"InvalidMaxDatastoreDst",
		"NsalistInvalidFormat",
		"NsalistDownloadTaskError",
		"NotEnoughSpaceInExtSaveData",
	};
	if (desc >= 1 && desc <= 77) return map[desc - 1];
	if (desc >= 192 && desc <= 192 + 6) return map2[desc - 192];
	return NULL;
}

static const char* get_desc_desc_boss(int desc) {
	const char* const map[] = {
		"The policy information pointer is NULL.",
		"The task action pointer is NULL.",
		"The task option pointer is NULL, or the option code is invalid.",
		"The pointer for getting the application list is NULL.",
		"The pointer for getting the task ID list is NULL.",
		"The pointer for getting the step ID list is NULL.",
		"The pointer to NS data list information is NULL.",
		"The task status pointer is NULL.",
		"The property value pointer is NULL.",
		"The pointer to the new arrival event is NULL.",
		"The pointer to the new arrival flag is NULL.",
		"The pointer to the OPTOUT flag is NULL.",
		"The pointer to the task error information is NULL.",
		"The pointer to the region that stores NSDATA is NULL.",
		"The pointer to the location that stores additional NSDATA information is NULL.",
		"The pointer to the storage region for the NSDATA read flag is NULL.",
		"The pointer to the region used to store the NSDATA update time is NULL.",
		"The pointer to the location used to store the minutes value of the next execution time is  NULL.",
		"The HTTP request header pointer is NULL.",
		"The pointer to information that can be used by the policy list is NULL.",
		"The pointer to information usable in test mode is NULL.",
		"The pointer to information on Parental Controls settings is NULL.",
		"The pointer to information about whether the user has agreed to the EULA is NULL.",
		"The pointer to the policy list's environment ID is NULL.",
		"The pointer to the policy list's URL information is NULL.",
		"The task ID pointer is NULL or a zero-length string.",
		"The current step ID was specified during task registration.",
		"The property type is not supported.",
		"The URL string pointer is NULL or a zero-length string.",
		"The file path string pointer is NULL or a zero-length string.",
		"The specified task priority is invalid.",
		"The task duration is invalid.",
		"The task action code is out of range.",
		"The NS data seek position exceeds the data size.",
		"The HTTP request header registration count exceeds the maximum.",
		"The number of client certificates set exceeds the maximum.",
		"The number of root CAs exceeds the maximum that can be set.",
		"The scheduling policy is invalid.",
		"The AP information type is invalid.",
		"The task permission information is invalid.",
		"The WaitFinish function timed out.",
		"The target task ended with a result other than TASK_DONE in the WaitFinish function.",
		"The IPC session is not initialized.",
		"There was a property size error during internal IPC processing.",
		"There are too many IPC requests. (This is provided for future extensibility and cannot be used.)",
		"IPC is already initialized.",
		"Insufficient memory.",
		"The maximum number of NSD files in the NSA was exceeded.",
		"An invalid NS data format was detected.",
		"The specified application ID was not found.",
		"The specified task ID was not found.",
		"The specified task step was not found.",
		"Another application of the same name is already registered.",
		"Another task of the same name is already registered.",
		"Another task step of the same name is already registered.",
		"A sequence error (such as an attempt to start a task that is currently running) was detected.",
		"Storage and tasks cannot be registered because the maximum number of registered application IDs and tasks has been reached.",
		"The task cannot be deleted because of its state (for example, it may currently be running or have already been scheduled).",
		"A task that is supposed to be registered in the database file was not found.",
		"Invalid file handle.",
		"An invalid keyword was detected in the scheduling policy list.",
		"More than the maximum number of option HTTP request headers were specified.",
		"An invalid header type was specified in the GetHeaderInfo function.",
		"You do not have storage access rights.",
		"Insufficient storage space.",
		"Storage has not been registered for the corresponding application ID.",
		"The specified NS data was not found.",
		"The header size does not match the header type specified by the GetHeaderInfo function.",
		"The NsDataIdList size is insufficient. (The list is not big enough to store all the NSD serial IDs.)",
		"The target NSD group for BOSS storage was updated since the last time a list was obtained.",
		"Not connected to an access point.",
		"Not connected to a network.",
		"Invalid friend code error.",
		"Failed to access a file. An abnormality occurred in the system save data when accessing the internal database and settings. This error cannot normally happen. The problem may disappear if you initialize the system save data, but doing so deletes all task information (including the information registered by other applications).",
		"Already paused.",
		"Already resumed.",
		"An unexpected error occurred.",	};
	const char* const map2[] = {
		"An invalid value was set in the parameter specified for storage.",
		"The configuration information type is out of range.",
		"The HTTP query registration count exceeds the maximum.",
		"The DataStore data transmission destination registration count exceeds the maximum.",
		"An error has been detected in the NSA list file format.",
		"The NSA list download result was an error.",
		"Expanded save data has insufficient capacity.",	};
	if (desc >= 1 && desc <= 77) return map[desc - 1];
	if (desc >= 192 && desc <= 192 + 6) return map2[desc - 192];
	return NULL;
}

static const char* get_desc_str_camera(int desc) {
	const char* const map[] = {
		"CameraIsSleeping",
		"CameraFatalError",
	};
	if (desc >= 1 && desc <= 2) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_camera(int desc) {
	const char* const map[] = {
		"Camera is sleeping",
		"Camera fatal error",
	};
	if (desc >= 1 && desc <= 2) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_cardspi(int desc) {
	const char* const map[] = {
		"NotPermitted",
		"CardIreqTimeOut",
		"CardIreqNotDetected",
		"InvalidArgument",
		"DeviceNotFound",
		"TimeOut",
	};
	if (desc >= 1 && desc <= 6) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_cardspi(int desc) {
	const char* const map[] = {
		"The operation is not permitted.",
		"The card interrupt request timed out.",
		"The card interrupt request was not detected.",
		"An invalid argument was passed.",
		"The device was not found.",
		"The operation timed out.",
	};
	if (desc >= 1 && desc <= 6) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_cec(int desc) {
	const char* const map[] = {
		"Unknown",
		"BoxSizeFull",
		"BoxMessNumFull",
		"BoxNumFull",
		"BoxAlreadyExists",
		"MessTooLarge",
		"InvalidData",
		"InvalidId",
		"NotAgreeEula",
		"ParentalControlCec",
		"FileAccessFailed",
		"SessionCanceled",
	};
	if (desc >= 100 && desc <= 111) return map[desc - 100];
	return NULL;
}

static const char* get_desc_desc_cec(int desc) {
	const char* const map[] = {
		"Unknown",
		"The box size is full.",
		"The number of messages in the box is full.",
		"The number of boxes is full.",
		"The box already exists.",
		"The message is too large.",
		"The data is invalid.",
		"The ID is invalid.",
		"User has not agreed to the EULA.",
		"User has not agreed due to Parental Controls.",
		"Failed.",
		"Session canceled.",
	};
	if (desc >= 100 && desc <= 111) return map[desc - 100];
	return NULL;
}

static const char* get_desc_str_cfg(int desc) {
	const char* const map[] = {
		"NotVerified",
		"VerificationFailed",
		"InvalidNtrSetting",
		"AlreadyLatestVersion",
		"MountContentFailed",
		"InvalidTarget",
	};
	if (desc >= 1 && desc <= 6) return map[desc - 1];
	if (desc == 1023) return "ObsoleteResult";
	return NULL;
}

static const char* get_desc_desc_cfg(int desc) {
	const char* const map[] = {
		"Signature not verified.",
		"Signature verification failed.",
		"A valid NTR setting was not found.",
		"Already the latest version.",
		"Failed to mount content.",
		"The target is invalid.",
	};
	if (desc >= 1 && desc <= 6) return map[desc - 1];
	if (desc == 1023) return "Obsolete; the error must be revised to another as soon as possible.";
	return NULL;
}

static const char* get_desc_str_csnd(int desc) {
	const char* const map[] = {
		"Sleep",
		"InferiorPriority",
	};
	if (desc >= 1 && desc <= 2) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_csnd(int desc) {
	const char* const map[] = {
		"The operation is not supported in the current state.",
		"The operation is not supported in the current state.",
	};
	if (desc >= 1 && desc <= 2) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_cup(int desc) {
	const char* const map[] = {
		"UpdateCancelled",
		"NotInitialized",
		"AlreadyInitialized",
		"AlreadyFinalized",
		"AlreadyFinished",
		"AlreadyCancelled",
		"AlreadyUpdating",
		"NotStarted",
		"NotSupported",
		"UpdatePartitionNotFound",
		"UpdateNotRequired",
		"InvalidUpdatePartitionFormat",
		"UpdateInfoNotFound",
	};
	if (desc >= 1 && desc <= 13) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_cup(int desc) {
	const char* const map[] = {
		"The update was cancelled.",
		"The update was not initialized.",
		"The update was already initialized.",
		"The update was already finalized.",
		"The update was already finished.",
		"The update was already cancelled.",
		"The update was already updating.",
		"The update was not started.",
		"The update is not supported.",
		"The update partition was not found.",
		"The update is not required.",
		"The update partition format is invalid.",
		"The update info was not found.",
	};
	if (desc >= 1 && desc <= 13) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_dbg(int desc) {
	const char* const map[] = {
		"DebugOutputIsDisabled",
		"DebuggerNotPresent",
		"InaccessiblePage",
	};
	if (desc >= 1 && desc <= 3) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_dbg(int desc) {
	const char* const map[] = {
		"Debug output is disabled",
		"Debugger is not connected",
		"An inaccessible page is included",
	};
	if (desc >= 1 && desc <= 3) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_fs(int desc);
static const char* get_desc_str_dbm(int desc) {
	switch (desc) {
		case 714:
		case 205:
		case 111:
		case 611:
		case 21:
		case 20:
		case 112:
		case 711:
		case 612:
		case 113:
		case 712:
		case 613:
		case 713:
		case 185:
		case 240:
		case 640:
		case 770:
		case 22:
		case 614:
		case 735:
		case 114:
		case 350:
		case 351:
			return get_desc_str_fs(desc);
	}
	return NULL;
}

static const char* get_desc_desc_fs(int desc);
static const char* get_desc_desc_dbm(int desc) {
	switch (desc) {
		case 714:
		case 205:
		case 111:
		case 611:
		case 21:
		case 20:
		case 112:
		case 711:
		case 612:
		case 113:
		case 712:
		case 613:
		case 713:
		case 185:
		case 240:
		case 640:
		case 770:
		case 22:
		case 614:
		case 735:
		case 114:
		case 350:
		case 351:
			return get_desc_desc_fs(desc);
	}
	return NULL;
}

static const char* get_desc_str_dd(int desc) {
	const char* const map[] = {
		"InvalidSelection",
		"InnaccessiblePage",
		"ManualResetEventRequired",
		"MaxHandle",
	};
	if (desc >= 1 && desc <= 4) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_dd(int desc) {
	const char* const map[] = {
		"Bad selection",
		"Contains inaccessible pages",
		"Manual reset event required",
		"maximum number of handles reached",
	};
	if (desc >= 1 && desc <= 4) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_dlp(int desc) {
	const char* const map[] = {
		"InternalState",
		"InternalError",
		"AlreadyOccupiedWirelessDevice",
		"WirelessOff",
		"NotFoundServer",
		"ServerIsFull",
		"InvalidAccessToMedia",
		"FailedToAccessMedia",
		"ChildTooLarge",
		"Incommutable",
		"InvalidDlpVersion",
		"InvalidRegion",
	};
	if (desc >= 1 && desc <= 12) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_dlp(int desc) {
	const char* const map[] = {
		"The internal state is innappropriate for using the API.",
		"An error occured that cannot be handled from the application.",
		"The wireless device is already occupied.",
		"State where communication is not possible.",
		"Cannot find the server.",
		"No more clients can connect to the server.",
		"The media to be accessed is not supported.",
		"Access to the media failed.",
		"Too much NAND memory is required to import the child program.",
		"Cannot communicate with the partner.",
		"The DLP version is invalid.",
		"The child program region differs from the region of the host.",
	};
	if (desc >= 1 && desc <= 12) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_dmnt(int desc) {
	const char* const map[] = {
		"MaxHandle",
		"InvalidHandle",
		"InvalidProcessId",
		"InvalidThreadId",
		"NotAuthorized",
		"Busy",
		"AlreadyDone",
		"ProcessTerminated",
		"NoEvent",
		"InnaccessiblePage",
		"InvalidDebugeeRegion",
		"Reboot",
		"InvalidArgument",
		"InvalidProgamId",
	};
	if (desc >= 1 && desc <= 14) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_dmnt(int desc) {
	const char* const map[] = {
		"maximum number of handles reached",
		"invalid handle",
		"invalid process ID",
		"invalid thread ID",
		"not authorized",
		"busy",
		"already done",
		"process terminated",
		"no event",
		"inaccessible page",
		"invalid debugee region",
		"reboot",
		"invalid argument",
		"invalid program ID",
	};
	if (desc >= 1 && desc <= 14) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_dsp(int desc) {
	const char* const map[] = {
		"OK",
		"ComponentNotLoaded",
	};
	if (desc >= 0 && desc <= 1) return map[desc];
	return NULL;
}

static const char* get_desc_desc_dsp(int desc) {
	const char* const map[] = {
		"The operation was successful.",
		"The component is not loaded.",	};
	if (desc >= 0 && desc <= 1) return map[desc];
	return NULL;
}

static const char* get_desc_str_ec(int desc) {
	const char* const map1[] = {
		"TooManyContents",
		"TooManyTitles",
		"NotInitialized",
		"TitleInvalid",
		"InvalidSelectionFilter",
		"NotInService",
		"NotImplemented",
		"DataTitleNotFound",
		"DataTitleContentInfoNotFound",
		"DataTitleContentNotFound",
		"MetaDataTitleNotFound",
		"MetaDataInvalidFormat",
		"MetaDataIconNotFound",
		"MetaDataContentNotFound",
		"OutOfFilterMemory",
		"CatalogInvalidArgument",
		"SessionPreparedTitleNotFound",
		"AppletCloseApplicationRequested",
		"ParentalControlShopUseRestricted",
		"ParentalControlWrongPinCode",
		"DataTitleNotOwned",
		"ServiceTitleNotOwned",
		"AccountNotCreated",
		"NotSupported",
		"TicketEnvelopeSizeInvalid",
		"DataTitleInUse",
		"OutOfCatalogMemory",
		"DataTitleContentNotDeletable",
		"TooManyItems",
		"PriceFormatInvalid",
	};
	const char* const map2[] = {
		"UpdatedVersionRetrieved",
		"SessionInvalid",
		"NewSessionRequired",
	};
	const char* const map3[] = {
		"EcardStatusRedeemedError",
		"EcardStatusRevokedError",
		"EcardStatusInactiveError",
		"EcardStatusUnknownError",
		"EcardTypeLegacyPointsError",
		"EcardInvalidIdError",
		"EcardTypeCacheError",
		"EcardTypeUnknownError",
		"EcardNoRedeemableItemsError",
		"FsMediaWriteProtectedError",
		"InfraReportError",
		"InfraReceiveError",
		"InfraInvalidResponseError",
		"InfraNeedsUpdateError",
		"InfraTimeoutError",
		"InfraBusyError",
		"InfraInMaintenanceError",
		"NetworkTimeoutError",
		"NetworkInMaintenanceError",
		"OutOfCatalogMemoryError",
		"OutOfSystemMemoryError",
		"FsMediaAccessFailedError",
		"NoEnoughSpaceError",
		"AcNotConnectedError",
		"AppletUnknownError",
		"UnknownError",
		"EcAppletNotFoundError",
	};
	const char* const map4[] = {
		"AppletCancelled",
		"AppletHomeButton",
		"AppletSoftwareReset",
		"AppletPowerButton",
		"AppletNotSupportedError",
		"AppletNotEnoughSpaceToDownloadContent",
		"AppletNecessaryAttributeNotFound",
		"AppletMetadataNotFound",
		"AppletTooManyContents",
		"AppletAlreadyPurchased",
		"AppletItemUnpurchasable",
		"AppletItemNotFound",
		"AppletAlreadyDownloaded",
		"AppletRedeemableItemNotFound",
		"AppletAlreadyRedeemedItem",
		"AppletShopServerError",
		"AppletImportFailed",
		"AppletWifiOffError",
		"AppletFailedConnectError",
		"AppletEcardUnavailable",
		"AppletInvalidRivtoken",
		"AppletInvalidParameter",
		"AppletFailed",
		"AppletFailedError",
		"AppletWrongPinCodeError",
		"AppletDuplicateContentIndex",
		"AppletFailedLoginError",
		"AppletDataTitleInUse",
	};
	const char* const map5[] = {
		"AppletNeedsDatatitleUpdate",
		"AppletDatatitleNotImported",
		"AppletRequiresReConnect",
		"AppletNeedsBalanceUpdate",
	};
	const char* const map6[] = {
		"AppletNeedsSystemUpdate",
		"AppletStandbyMode",
		"AppletShopServiceTerminated",
		"AppletShopServiceUnavailable",
		"AppletCountryChanged",
		"AppletNotAgreeEulaError",
		"AppletTooManyDatatitles",
		"AppletNotEnoughSpaceToDownloadDatatitle",
		"AppletSdNotInserted",
		"AppletSdWriteProtected",
		"AppletSdBroken",
		"AppletSdEjected",
		"AppletInvalidVersion",
		"AppletSdAccessError",
	};
	if (desc >= 128 && desc <= 128 + 29) return map1[desc - 128];
	if (desc >= 192 && desc <= 192 + 2) return map2[desc - 192];
	if (desc >= 256 && desc <= 256 + 26) return map3[desc - 256];
	if (desc == 320 - 2) return "InfraNeedsReconnectError";
	if (desc >= 1 && desc <= 28) return map4[desc - 1];
	if (desc >= 64 && desc <= 64 + 3) return map5[desc - 64];
	if (desc >= 96 && desc <= 96 + 13) return map6[desc - 96];
	return NULL;
}

static const char* get_desc_desc_ec(int desc) {
	const char* const map1[] = {
		"Too many contents.",
		"Too many titles.",
		"Not initialized.",
		"Title invalid.",
		"Invalid selection filter.",
		"Not in service.",
		"Not implemented.",
		"Data title not found.",
		"Data title content info not found.",
		"Data title content not found.",
		"Meta data title not found.",
		"Meta data invalid format.",
		"Meta data icon not found.",
		"Meta data content not found.",
		"Out of filter memory.",
		"Catalog invalid argument.",
		"Session prepared title not found.",
		"Applet close application requested.",
		"Parental control shop use restricted.",
		"Parental control wrong pin code.",
		"Data title not owned.",
		"Service title not owned.",
		"Account not created.",
		"Not supported.",
		"Ticket envelope size invalid.",
		"Data title in use.",
		"Out of catalog memory.",
		"Data title content not deletable.",
		"Too many items.",
		"Price format invalid.",
	};
	const char* const map2[] = {
		"Updated version retrieved.",
		"Session invalid.",
		"New session required.",
	};
	const char* const map3[] = {
		"E-CARD status redeemed error.",
		"E-CARD status revoked error.",
		"E-CARD status inactive error.",
		"E-CARD status unknown error.",
		"E-CARD type legacy points error.",
		"E-CARD invalid ID error.",
		"E-CARD type cache error.",
		"E-CARD type unknown error.",
		"E-CARD no redeemable items error.",
		"FS media write protected error.",
		"Infra report error.",
		"Infra receive error.",
		"Infra invalid response error.",
		"Infra needs update error.",
		"Infra timeout error.",
		"Infra busy error.",
		"Infra in maintenance error.",
		"Network timeout error.",
		"Network in maintenance error.",
		"Out of catalog memory error.",
		"Out of system memory error.",
		"FS media access failed error.",
		"No enough space error.",
		"AC not connected error.",
		"Applet unknown error.",
		"Unknown error.",
		"EC applet not found error.",
	};
	const char* const map4[] = {
		"Applet cancelled.",
		"Applet home button.",
		"Applet software reset.",
		"Applet power button.",
		"Applet not supported error.",
		"Applet not enough space to download content.",
		"Applet necessary attribute not found.",
		"Applet metadata not found.",
		"Applet too many contents.",
		"Applet already purchased.",
		"Applet item unpurchasable.",
		"Applet item not found.",
		"Applet already downloaded.",
		"Applet redeemable item not found.",
		"Applet already redeemed item.",
		"Applet shop server error.",
		"Applet import failed.",
		"Applet wifi off error.",
		"Applet failed connect error.",
		"Applet E-CARD unavailable.",
		"Applet invalid RIV token.",
		"Applet invalid parameter.",
		"Applet failed.",
		"Applet failed error.",
		"Applet wrong pin code error.",
		"Applet duplicate content index.",
		"Applet failed login error.",
		"Applet data title in use.",
	};
	const char* const map5[] = {
		"Applet needs data title update.",
		"Applet data title not imported.",
		"Applet requires reconnect.",
		"Applet needs balance update.",
	};
	const char* const map6[] = {
		"Applet needs system update.",
		"Applet standby mode.",
		"Applet shop service terminated.",
		"Applet shop service unavailable.",
		"Applet country changed.",
		"Applet not agree EULA error.",
		"Applet too many data titles.",
		"Applet not enough space to download data title.",
		"Applet SD not inserted.",
		"Applet SD write protected.",
		"Applet SD broken.",
		"Applet SD ejected.",
		"Applet invalid version.",
		"Applet SD access error.",
	};
	if (desc >= 128 && desc <= 128 + 29) return map1[desc - 128];
	if (desc >= 192 && desc <= 192 + 2) return map2[desc - 192];
	if (desc >= 256 && desc <= 256 + 26) return map3[desc - 256];
	if (desc == 320 - 2) return "Infra needs reconnect error.";
	if (desc >= 1 && desc <= 28) return map4[desc - 1];
	if (desc >= 64 && desc <= 64 + 3) return map5[desc - 64];
	if (desc >= 96 && desc <= 96 + 13) return map6[desc - 96];
	return NULL;
}

static const char* get_desc_str_enc(int desc) {
	const char* const map[] = {
		"NoBufferLeft",
		"InvalidParameter",
		"InvalidFormat",
	};
	if (desc >= 1 && desc <= 3) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_enc(int desc) {
	const char* const map[] = {
		"No buffer left.",
		"Invalid parameter.",
		"Invalid format.",
	};
	if (desc >= 1 && desc <= 3) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_fnd(int desc) {
	if (desc == 33) return "InvalidTlsIndex";
	return NULL;
}

static const char* get_desc_desc_fnd(int desc) {
	if (desc == 33) return "An unallocated TLS index was specified.";
	return NULL;
}

static const char* get_desc_str_friends(int desc) {
	int map_picker = CTR_RESULT_GET_CODE_BITS(desc, 0b111100000, 5);
	int map_val = CTR_RESULT_GET_CODE_BITS(desc, 0b11111, 5);
	const char* const map1[] = {
		"CoreSuccess",
		"CoreSuccessPending",
		"CoreUnknown",
		"CoreNotImplemented",
		"CoreInvalidPointer",
		"CoreOperationAborted",
		"CoreException",
		"CoreAccessDenied",
		"CoreInvalidHandle",
		"CoreInvalidIndex",
		"CoreOutOfMemory",
		"CoreInvalidArgument",
		"CoreTimeout",
		"CoreInitializationFailure",
		"CoreCallInitiationFailure",
		"CoreRegistrationError",
		"CoreBufferOverflow",
		"CoreInvalidLockState",
		"CoreUndefined",
	};
	const char* const map2[] = {
		"DdlSuccess",
		"DdlInvalidSignature",
		"DdlIncorrectVersion",
		"DdlUndefined",
	};
	const char* const map3[] = {
		"RendezvousSuccess",
		"RendezvousConnectionFailure",
		"RendezvousNotAuthenticated",
		"RendezvousInvalidUsername",
		"RendezvousInvalidPassword",
		"RendezvousUsernameAlreadyExists",
		"RendezvousAccountDisabled",
		"RendezvousAccountExpired",
		"RendezvousConcurrentLoginDenied",
		"RendezvousEncryptionFailure",
		"RendezvousInvalidPid",
		"RendezvousMaxConnectionsReached",
		"RendezvousInvalidGid",
		"RendezvousInvalidThreadId",
		"RendezvousInvalidOperationInLiveEnvironment",
		"RendezvousDuplicateEntry",
		"RendezvousControlScriptFailure",
		"RendezvousClassNotFound",
		"RendezvousSessionVoid",
		"RendezvousLspGatewayUnreachable",
		"RendezvousDdlMismatch",
		"RendezvousInvalidFtpInfo",
		"RendezvousSessionFull",
		"RendezvousUndefined",
	};
	const char* const map4[] = {
		"PythonCoreSuccess",
		"PythonCoreException",
		"PythonCoreTypeError",
		"PythonCoreIndexError",
		"PythonCoreInvalidReference",
		"PythonCoreCallFailure",
		"PythonCoreMemoryError",
		"PythonCoreKeyError",
		"PythonCoreOperationError",
		"PythonCoreConversionError",
		"PythonCoreValidationError",
		"PythonCoreUndefined",
	};
	const char* const map5[] = {
		"TransportSuccess",
		"TransportUnknown",
		"TransportConnectionFailure",
		"TransportInvalidUrl",
		"TransportInvalidKey",
		"TransportInvalidUrlType",
		"TransportDuplicateEndpoint",
		"TransportIoError",
		"TransportTimeout",
		"TransportConnectionReset",
		"TransportIncorrectRemoteAuthentication",
		"TransportServerRequestError",
		"TransportDecompressionFailure",
		"TransportCongestedEndpoint",
		"TransportUpnpCannotInit",
		"TransportUpnpCannotAddMapping",
		"TransportNatPmpCannotInit",
		"TransportNatPmpCannotAddMapping",
		"TransportUndefined",
	};
	const char* const map6[] = {
		"DoCoreSuccess",
		"DoCoreCallPostponed",
		"DoCoreStationNotReached",
		"DoCoreTargetStationDisconnect",
		"DoCoreLocalStationLeaving",
		"DoCoreObjectNotFound",
		"DoCoreInvalidRole",
		"DoCoreCallTimeout",
		"DoCoreRmcDispatchFailed",
		"DoCoreMigrationInProgress",
		"DoCoreNoAuthority",
		"DoCoreNoTargetStationSpecified",
		"DoCoreJoinFailed",
		"DoCoreJoinDenied",
		"DoCoreConnectivityTestFailed",
		"DoCoreUnknown",
		"DoCoreUnfreedReferences",
		"DoCoreUndefined",
	};
	const char* const map7[] = {
		"FpdSuccess",
		"RmcNotCalled",
		"DaemonNotInitialized",
		"DaemonAlreadyInitialized",
		"NotConnected",
		"Connected",
		"InitializationFailure",
		"OutOfmemory",
		"RmcFailed",
		"InvalidArgument",
		"InvalidLocalAccountId",
		"InvalidPrincipalId",
		"InvalidLocalFriendCode",
		"LocalAccountNotExists",
		"LocalAccountNotLoaded",
		"LocalAccountAlreadyLoaded",
		"FriendAlreadyExists",
		"FriendNotExists",
		"FriendNumMax",
		"NotFriend",
		"FileIoError",
		"P2pInternetProhibited",
		"Unknown",
		"InvalidState",
		"MiiNotExists",
		"AddFriendProhibited",
		"InvalidReference",
		"FpdUndefined",
	};
	const char* const map8[] = {
		"AuthenticationSuccess",
		"AuthenticationNasAuthenticateError",
		"AuthenticationTokenParseError",
		"AuthenticationHttpConnectionError",
		"AuthenticationHttpDnsError",
		"AuthenticationHttpGetProxySetting",
		"AuthenticationTokenExpired",
		"AuthenticationValidationFailed",
		"AuthenticationInvalidParam",
		"AuthenticationPrincipalIdUnmatched",
		"AuthenticationMoveCountUnmatch",
		"AuthenticationUnderMaintenance",
		"AuthenticationUnsupportedVersion",
		"AuthenticationUnknown",
		"AuthenticationUndefined",
	};
	const char* const map9[] = {
		"InvalidFriendCode",
		"NotLoggedIn",
		"NotFriendsResult",
		"UndefinedFacility",
		"AcNotConnected",
	};
	const char* const* const map_meta[8] = {map1, map2, map3, map4, map5, map6, map7, map8};
	const int map_max_num[8] = {17, 2, 22, 10, 17, 16, 26, 13};
	if (map_picker >= 1 && map_picker <= 8) {
		int max_num = map_max_num[map_picker - 1];
		if (map_val >= 0 && map_val <= max_num) return map_meta[map_picker - 1][map_val];
		if (map_val == 0b11111) return map_meta[map_picker - 1][max_num + 1];
	}
	if (desc >= 1 && desc <= 5) return map9[desc - 1];
	return NULL;
}

static const char* get_desc_desc_friends(int desc) {
	int map_picker = CTR_RESULT_GET_CODE_BITS(desc, 0b111100000, 5);
	int map_val = CTR_RESULT_GET_CODE_BITS(desc, 0b11111, 5);
	const char* const map1[] = {
		"The friends-core operation was successful.",
		"The friends-core operation was successful, but the result is pending.",
		"An unknown error occurred in the friends-core operation.",
		"The friends-core operation is not implemented.",
		"The pointer passed to the friends-core operation is invalid.",
		"The friends-core operation was aborted.",
		"An exception occurred in the friends-core operation.",
		"Access was denied in the friends-core operation.",
		"The handle passed to the friends-core operation is invalid.",
		"The index passed to the friends-core operation is invalid.",
		"The friends-core operation ran out of memory.",
		"The argument passed to the friends-core operation is invalid.",
		"The friends-core operation timed out.",
		"The friends-core operation failed to initialize.",
		"The friends-core operation failed to initiate a call.",
		"The friends-core operation failed to register.",
		"The friends-core operation overflowed a buffer.",
		"The lock state passed to the friends-core operation is invalid.",
		"The friends-core operation returned an undefined error.",
	};
	const char* const map2[] = {
		"The friends-ddl operation was successful.",
		"The signature passed to the friends-ddl operation is invalid.",
		"The version passed to the friends-ddl operation is incorrect.",
		"The friends-ddl operation returned an undefined error.",
	};
	const char* const map3[] = {
		"The friends-rendezvous operation was successful.",
		"The friends-rendezvous operation failed to connect.",
		"The friends-rendezvous operation is not authenticated.",
		"The username passed to the friends-rendezvous operation is invalid.",
		"The password passed to the friends-rendezvous operation is invalid.",
		"The username passed to the friends-rendezvous operation already exists.",
		"The account passed to the friends-rendezvous operation is disabled.",
		"The account passed to the friends-rendezvous operation is expired.",
		"The concurrent login passed to the friends-rendezvous operation is denied.",
		"The encryption passed to the friends-rendezvous operation failed.",
		"The pid passed to the friends-rendezvous operation is invalid.",
		"The max connections passed to the friends-rendezvous operation is reached.",
		"The gid passed to the friends-rendezvous operation is invalid.",
		"The thread id passed to the friends-rendezvous operation is invalid.",
		"The operation passed to the friends-rendezvous operation is invalid in live environment.",
		"The entry passed to the friends-rendezvous operation is duplicate.",
		"The control script passed to the friends-rendezvous operation failed.",
		"The class passed to the friends-rendezvous operation is not found.",
		"The session passed to the friends-rendezvous operation is void.",
		"The lsp gateway passed to the friends-rendezvous operation is unreachable.",
		"The ddl passed to the friends-rendezvous operation is mismatch.",
		"The ftp info passed to the friends-rendezvous operation is invalid.",
		"The session passed to the friends-rendezvous operation is full.",
		"The friends-rendezvous operation returned an undefined error.",
	};
	const char* const map4[] = {
		"The friends-pythoncore operation was successful.",
		"An exception occurred in the friends-pythoncore operation.",
		"The type passed to the friends-pythoncore operation is invalid.",
		"The index passed to the friends-pythoncore operation is invalid.",
		"The reference passed to the friends-pythoncore operation is invalid.",
		"The call passed to the friends-pythoncore operation failed.",
		"The memory passed to the friends-pythoncore operation is invalid.",
		"The key passed to the friends-pythoncore operation is invalid.",
		"The operation passed to the friends-pythoncore operation is invalid.",
		"The conversion passed to the friends-pythoncore operation is invalid.",
		"The validation passed to the friends-pythoncore operation is invalid.",
		"The friends-pythoncore operation returned an undefined error.",
	};
	const char* const map5[] = {
		"The friends-transport operation was successful.",
		"An unknown error occurred in the friends-transport operation.",
		"The connection passed to the friends-transport operation failed.",
		"The url passed to the friends-transport operation is invalid.",
		"The key passed to the friends-transport operation is invalid.",
		"The url type passed to the friends-transport operation is invalid.",
		"The endpoint passed to the friends-transport operation is duplicate.",
		"The io passed to the friends-transport operation is invalid.",
		"The timeout passed to the friends-transport operation is invalid.",
		"The connection reset passed to the friends-transport operation is invalid.",
		"The remote authentication passed to the friends-transport operation is incorrect.",
		"The server request passed to the friends-transport operation is invalid.",
		"The decompression passed to the friends-transport operation failed.",
		"The endpoint passed to the friends-transport operation is congested.",
		"The upnp passed to the friends-transport operation cannot init.",
		"The upnp passed to the friends-transport operation cannot add mapping.",
		"The nat pmp passed to the friends-transport operation cannot init.",
		"The nat pmp passed to the friends-transport operation cannot add mapping.",
		"The friends-transport operation returned an undefined error.",
	};
	const char* const map6[] = {
		"The friends-docore operation was successful.",
		"The call passed to the friends-docore operation is postponed.",
		"The station passed to the friends-docore operation is not reached.",
		"The target station passed to the friends-docore operation is disconnected.",
		"The local station passed to the friends-docore operation is leaving.",
		"The object passed to the friends-docore operation is not found.",
		"The role passed to the friends-docore operation is invalid.",
		"The call passed to the friends-docore operation timed out.",
		"The rmc passed to the friends-docore operation dispatch failed.",
		"The migration passed to the friends-docore operation is in progress.",
		"The authority passed to the friends-docore operation is invalid.",
		"The target station passed to the friends-docore operation is not specified.",
		"The join passed to the friends-docore operation failed.",
		"The join passed to the friends-docore operation is denied.",
		"The connectivity test passed to the friends-docore operation failed.",
		"An unknown error occurred in the friends-docore operation.",
		"The references passed to the friends-docore operation are unfreed.",
		"The friends-docore operation returned an undefined error.",
	};
	const char* const map7[] = {
		"The friends-fpd operation was successful.",
		"The rmc passed to the friends-fpd operation is not called.",
		"The daemon passed to the friends-fpd operation is not initialized.",
		"The daemon passed to the friends-fpd operation is already initialized.",
		"The connection passed to the friends-fpd operation is not connected.",
		"The connection passed to the friends-fpd operation is connected.",
		"The initialization passed to the friends-fpd operation failed.",
		"The memory passed to the friends-fpd operation is out of memory.",
		"The rmc passed to the friends-fpd operation failed.",
		"The argument passed to the friends-fpd operation is invalid.",
		"The local account id passed to the friends-fpd operation is invalid.",
		"The principal id passed to the friends-fpd operation is invalid.",
		"The local friend code passed to the friends-fpd operation is invalid.",
		"The local account passed to the friends-fpd operation does not exist.",
		"The local account passed to the friends-fpd operation is not loaded.",
		"The local account passed to the friends-fpd operation is already loaded.",
		"The friend passed to the friends-fpd operation already exists.",
		"The friend passed to the friends-fpd operation does not exist.",
		"The friend passed to the friends-fpd operation is max.",
		"The friend passed to the friends-fpd operation is not a friend.",
		"The io passed to the friends-fpd operation is invalid.",
		"The internet passed to the friends-fpd operation is prohibited.",
		"An unknown error occurred in the friends-fpd operation.",
		"The state passed to the friends-fpd operation is invalid.",
		"The mii passed to the friends-fpd operation does not exist.",
		"The friend passed to the friends-fpd operation is prohibited.",
		"The reference passed to the friends-fpd operation is invalid.",
		"The friends-fpd operation returned an undefined error.",
	};
	const char* const map8[] = {
		"The friends-authentication operation was successful.",
		"The network authentication server passed to the friends-authentication operation is invalid.",
		"The token passed to the friends-authentication operation is invalid.",
		"The http connection passed to the friends-authentication operation is invalid.",
		"The http dns passed to the friends-authentication operation is invalid.",
		"The http get proxy passed to the friends-authentication operation is invalid.",
		"The token passed to the friends-authentication operation is expired.",
		"The validation passed to the friends-authentication operation is invalid.",
		"The param passed to the friends-authentication operation is invalid.",
		"The principal id passed to the friends-authentication operation is unmatched.",
		"The move count passed to the friends-authentication operation is unmatch.",
		"The maintenance passed to the friends-authentication operation is under maintenance.",
		"The version passed to the friends-authentication operation is unsupported.",
		"An unknown error occurred in the friends-authentication operation.",
		"The friends-authentication operation returned an undefined error.",
	};
	const char* const map9[] = {
		"The friend code passed to the friends operation is invalid.",
		"The login passed to the friends operation is not logged in.",
		"The result passed to the friends operation is not friends.",
		"The facility passed to the friends operation is undefined.",
		"The connection passed to the friends operation is not connected.",
	};
	const char* const* const map_meta[8] = {map1, map2, map3, map4, map5, map6, map7, map8};
	const int map_max_num[8] = {17, 2, 22, 10, 17, 16, 26, 13};
	if (map_picker >= 1 && map_picker <= 8) {
		int max_num = map_max_num[map_picker - 1];
		if (map_val >= 0 && map_val <= max_num) return map_meta[map_picker - 1][map_val];
		if (map_val == 0b11111) return map_meta[map_picker - 1][max_num + 1];
	}
	if (desc >= 1 && desc <= 5) return map9[desc - 1];
	return NULL;
}

static const char* get_desc_str_fs(int desc) {
	switch (desc) {
		case 10: return "FindFinished";
		case 20: return "DbmFindFinished";
		case 21: return "DbmFindKeyFinished";
		case 22: return "DbmIterationFinished";

		case 100: return "NotFound";
		case 101: return "ArchiveNotFound";
		case 102: return "CardPartitionNotFound";
		case 110: return "DbmNotFound";
		case 111: return "DbmKeyNotFound";
		case 112: return "DbmFileNotFound";
		case 113: return "DbmDirectoryNotFound";
		case 114: return "DbmIdNotFound";
		case 120: return "FatNotFound";
		case 130: return "MediaNotFound";
		case 135: return "FatNoStorage";
		case 140: return "CardRomNotFound";
		case 141: return "CardRomNoDevice";
		case 142: return "CardRomUnkown";
		case 150: return "CardBackupNotFound";
		case 151: return "CardBackupType1NoDevice";
		case 152: return "CardBackupType2NoDevice";
		case 153: return "CardBackupType1Unkown";
		case 154: return "CardBackupType2Unkown";
		case 170: return "SdmcNotFound";
		case 171: return "SdmcNoDevice";
		case 172: return "SdmcUnkown";

		case 180: return "AlreadyExists";
		case 185: return "DbmAlreadyExists";
		case 190: return "FatAlreadyExists";

		case 200: return "NotEnoughSpace";
		case 205: return "DbmNotEnoughSpace";
		case 210: return "FatNotEnoughSpace";

		case 220: return "ArchiveInvalidated";

		case 230: return "OperationDenied";
		case 231: return "ResourceLocked";
		case 235: return "CacheError";
		case 240: return "DbmOperationDenied";
		case 250: return "FatOperationDenied";
		case 260: return "WriteProtected";
		case 265: return "SdmcWriteProtected";
		case 270: return "CardWriteProtected";
		case 280: return "MediaAccessError";
		case 290: return "CardRomAccessError";
		case 291: return "CardRomCommError";
		case 300: return "CardBackupCommError";
		case 301: return "CardBackupType1CommError";
		case 302: return "CardBackupType2CommError";
		case 320: return "NandAccessError";
		case 321: return "NandCommError";
		case 322: return "NandEccFailed";
		case 330: return "SdmcAccessError";
		case 331: return "SdmcCommError";
		case 332: return "SdmcEccFailed";
		case 335: return "BackupNotRequired";

		case 340: return "NotFormatted";
		case 341: return "FormatIsFactoryDefaults";
		case 350: return "DbmNotFormatted";
		case 351: return "DbmVersionMismatch";

		case 360: return "BadFormat";
		case 370: return "FatBadFormat";
		case 371: return "FatBrokenEntry";
		case 380: return "CardBackupBadFormat";
		case 381: return "CardBackupType1BadFormat";
		case 382: return "CardBackupType2BadFormat";

		case 390: return "VerificationFailed";
		case 391: return "HashMismatch";
		case 392: return "VerifySignatureFailed";
		case 393: return "InvalidFileFormat";
		case 394: return "VerifyHeaderMacFailed";
		case 395: return "VerifyDataHashFailed";

		case 560: return "ProgramDataNotFound";
		case 561: return "ProgramNotFound";
		case 562: return "SystemMenuDataNotFound";
		case 563: return "BannerDataNotFound";
		case 564: return "LogoDataNotFound";
		case 565: return "ProgramInfoNotFound";
		case 566: return "ContentNotFound";
		case 567: return "NoSuchExeFsSection";

		case 580: return "RequestRetry";
		case 581: return "RequestSameRetry";
		case 582: return "RequestSmallRetry";

		case 590: return "InvalidProgramFormat";
		case 591: return "InvalidCxiFormat";
		case 592: return "InvalidCciFormat";

		case 600: return "OutOfResource";
		case 601: return "OutOfMemory";
		case 602: return "HandleTableFull";
		case 610: return "DbmOutOfResource";
		case 611: return "DbmTableFull";
		case 612: return "DbmFileEntryFull";
		case 613: return "DbmDirectoryEntryFull";
		case 614: return "DbmOutOfMemory";
		case 620: return "FatOutOfResource";
		case 621: return "FatOutOfMemory";

		case 630: return "AccessDenied";
		case 631: return "NotDevelopmentId";
		case 632: return "NoContentRights";
		case 640: return "DbmAccessDenied";
		case 650: return "FatAccessDenied";

		case 700: return "InvalidArgument";
		case 701: return "InvalidPositionBase";
		case 702: return "InvalidPathFormat";
		case 703: return "PathTooLong";
		case 704: return "OutOfBounds";
		case 705: return "InvalidPosition";
		case 710: return "DbmInvalidArgument";
		case 711: return "DbmFileNameTooLong";
		case 712: return "DbmDirectoryNameTooLong";
		case 713: return "DbmInvalidPathFormat";
		case 714: return "DbmOutOfBounds";
		case 720: return "FatInvalidArgument";
		case 721: return "FatInvalidSize";

		case 730: return "NotInitialized";
		case 731: return "LibraryNotInitialized";
		case 732: return "MovableDataIsNotEnabled";
		case 735: return "DbmNotInitialized";
		case 740: return "CardRomNotInitialized";

		case 750: return "AlreadyInitialized";

		case 760: return "UnsupportedOperation";
		case 761: return "UnsupportedAlignment";
		case 762: return "UnsupportedMedia";
		case 770: return "DbmInvalidOperation";

		case 900: return "NotImplemented";
		case 901: return "InvalidKeyType";
		case 902: return "PartitionNotFound";

		case 910: return "FatError";
		case 911: return "FatCorruption";

		case 920: return "NandError";
		case 921: return "NandFatal";

		case 930: return "UnknownError";
	}
	return NULL;
}

static const char* get_desc_desc_fs(int desc) {
	switch (desc) {
		case 10: return "Search finished";
		case 20: return "DBM search completed";
		case 21: return "DBM Key lookup completed.";
		case 22: return "DBM iteration is complete.";

		case 100: return "File, archive, etc. not found.";
		case 101: return "Archive not found";
		case 102: return "Card partition not found";
		case 110: return "DBM can't find files, archives, etc.";
		case 111: return "DBM key not found";
		case 112: return "DBM file not found";
		case 113: return "DBM directory not found";
		case 114: return "DBM ID not found";
		case 120: return "Specified FAT volume or path does not exist.";
		case 130: return "Media not found or not recognized.";
		case 135: return "No accessible FAT device exists.";
		case 140: return "The card media cannot be found/recognized.";
		case 141: return "The card is not inserted or is missing.";
		case 142: return "Bad card (wrong ID).";
		case 150: return "The backup card media cannot be found/recognized.";
		case 151: return "Card Type1 backup device is not inserted or removed.";
		case 152: return "Card Type2 backup device is not inserted or removed.";
		case 153: return "Bad card Type1 backup device (ID bad).";
		case 154: return "Bad card Type1 backup device (ID bad).";
		case 170: return "The SD card cannot be found/recognized.";
		case 171: return "The SD card is not inserted or is missing.";
		case 172: return "A medium other than an SD card is inserted (MMC, etc.).";

		case 180: return "Files, archives, etc. already exists.";
		case 185: return "The specified DBM file already exists.";
		case 190: return "The specified FAT path already exists.";

		case 200: return "No space left";
		case 205: return "DBM storage was full.";
		case 210: return "Insufficient space on FAT device.";

		case 220: return "Archiving disabled.";

		case 230: return "Operation denied";
		case 231: return "Resource locked";
		case 235: return "An error occurred during a cache operation.";
		case 240: return "DBM operation denied";
		case 250: return "FAT operation denied";
		case 260: return "Writing is prohibited.";
		case 265: return "The SD card is locked (write-protected).";
		case 270: return "The card is locked (write-protected).";
		case 280: return "An error occurred while accessing the media.";
		case 290: return "Error accessing card rom media.";
		case 291: return "Card communication error (such as CRC error).";
		case 300: return "Error accessing card backup media.";
		case 301: return "Card Type1 backup device communication error (such as CRC error).";
		case 302: return "Card Type2 backup device communication error (such as CRC error).";
		case 320: return "Error accessing NAND media.";
		case 321: return "NAND communication error (CRC error, etc.)";
		case 322: return "Data lost due to NAND ECC uncorrectable.";
		case 330: return "Error accessing SD card media.";
		case 331: return "SD card communication error (such as CRC error).";
		case 332: return "Data lost due to SD card ECC uncorrectable.";
		case 335: return "Save data does not exist.";

		case 340: return "Not formatted";
		case 341: return "Factory default.";
		case 350: return "DBM not formatted";
		case 351: return "Different DBM data versions.";

		case 360: return "Invalid format";
		case 370: return "Bad FAT format";
		case 371: return "The specified FAT path target is broken.";
		case 380: return "Bad card backup format";
		case 381: return "Invalid card type1 backup device format";
		case 382: return "Invalid card type2 backup device format";

		case 390: return "Validation failed or tampering detected";
		case 391: return "Hash mismatch";
		case 392: return "Signature verification failed";
		case 393: return "Invalid file format";
		case 394: return "Verification of the header MAC failed";
		case 395: return "Hash verification of real data failed";

		case 560: return "Program, data in program not found.";
		case 561: return "Program not found";
		case 562: return "Does not contain icon data.";
		case 563: return "Does not contain banner data.";
		case 564: return "Does not contain logo data.";
		case 565: return "ProgramInfo not found";
		case 566: return "Content not found";
		case 567: return "Specified section not found.";

		case 580: return "Request retry";
		case 581: return "Retry request from SDMC device.";
		case 582: return "Retry request from card device.";

		case 590: return "Invalid program format";
		case 591: return "Malformed CXI file";
		case 592: return "Malformed CCI file";

		case 600: return "Insufficient resources. This error should not occur in the product.";
		case 601: return "Failed to allocate memory.";
		case 602: return "There is no space in the handle table.";
		case 610: return "Insufficient DBM resources.";
		case 611: return "The DBM table is full.";
		case 612: return "The DBM file entry is full.";
		case 613: return "The DBM directory entry is full.";
		case 614: return "DBM failed to allocate memory.";
		case 620: return "Insufficient FAT resources.";
		case 621: return "FAT failed to allocate memory.";

		case 630: return "You don't have access. This error should not occur in the product.";
		case 631: return "Invalid program ID.";
		case 632: return "You don't have permission to start the program.";
		case 640: return "DBM access denied";
		case 650: return "FAT access denied";

		case 700: return "Invalid argument. This error should not occur in the product.";
		case 701: return "Bad seek position.";
		case 702: return "Bad path.";
		case 703: return "Path too long";
		case 704: return "Operation out of bounds";
		case 705: return "Bad position in archive.";
		case 710: return "Bad DBM argument.";
		case 711: return "DBM file name too long";
		case 712: return "DBM directory name too long";
		case 713: return "Bad DBM path";
		case 714: return "DBM index out of range.";
		case 720: return "Bad FAT argument.";
		case 721: return "Bad FAT size";

		case 730: return "A process that requires initialization was called before it was initialized. This error should not occur in the product.";
		case 731: return "File system library not initialized";
		case 732: return "Movable data is not initialized.";
		case 735: return "DBM not initialized";
		case 740: return "Card not initialized";

		case 750: return "Initialization processing was executed in an already initialized state. This error should not occur in the product.";

		case 760: return "Unsupported function or operation not allowed. This error should not occur in the product.";
		case 761: return "Unsupported alignment size";
		case 762: return "This feature is not supported for the specified media.";
		case 770: return "Illegal DBM operation";

		case 900: return "Feature not implemented.";
		case 901: return "An incorrect encryption key is being used.";
		case 902: return "A partition that should exist is not found.";

		case 910: return "FAT error";
		case 911: return "FAT is in an invalid state.";

		case 920: return "NAND error";
		case 921: return "NAND may be faulty.";

		case 930: return "Unknown error";
	}
	return NULL;
}

static const char* get_desc_str_gd(int desc) {
	const char* const map[] = {
		"Success",
		"InvalidParameter",
		"NullParameter",
		"OutOfRange",
		"OutOfMemory",
		"ExtMemoryAllocationFailed",
		"InvalidMemoryRegion",
		"InvalidFunctionCall",
		"AlreadyReleased",
		"InputlayoutInvalidStreamSlots",
		"Texture2dInvalidResolution",
		"Texture2dInvalidSubregionResolution",
		"Texture2dInvalidFormat",
		"Texture2dInvalidMemoryLayout",
		"Texture2dInvalidMemoryLocation",
		"Texture2dInvalidMiplevelIndex",
		"Texture2dInvalidMiplevelIndexForMipmapAutogeneration",
		"Texture2dInvalidFormatForCreatingRenderTarget",
		"Texture2dInvalidFormatForCreatingDepthStencilTarget",
		"DifferentRenderTargetAndDepthStencilTargetResolution",
		"Texture2dInvalidTextureUnitId",
		"Texture2dInvalidOffset",
		"ResourceAlreadyMapped",
		"ResourceWasNotMapped",
		"NoTextureBound",
		"NoTextureCoordinates",
		"InvalidShaderUniformName",
		"InvalidShaderUniform",
		"InvalidShaderSignature",
		"InvalidShaderOperation",
		"GeometryShaderIncompatibleWithImmediateDraw",
		"SystemNoCmdListBind",
		"SystemInvalidCmdListBind",
		"SystemReceiveErrorFromGlgetError",
		"SystemNoPacketToRecord",
		"SystemNoPacketRecorded",
		"SystemAPacketIsAlreadyBeingRecorded",
		"SystemInvalidPacketId",
		"SystemRequestListInsertionIncompatibleWithJump",
	};
	if (desc >= 0 && desc <= 38) return map[desc];
	return NULL;
}

static const char* get_desc_desc_gd(int desc) {
	const char* const map[] = {
		"Represents success.",
		"A parameter is invalid.",
		"NULL has been specified for a parameter.",
		"A parameter is out of range.",
		"Insufficient FCRAM memory region.",
		"Insufficient VRAM memory region.",
		"The memory region is invalid.",
		"Incorrect function call.",
		"The resource has already been released.",
		"The input layout stream slots are invalid.",
		"Invalid 2D texture resolution.",
		"The region is invalid.",
		"Invalid 2D texture format.",
		"The memory layout is invalid.",
		"The memory location is invalid.",
		"Invalid 2D texture mipmap level index.",
		"Invalid auto-generated 2D texture mipmap level index.",
		"Invalid 2D texture format for creating the render target.",
		"Invalid 2D texture format for creating the depth stencil target.",
		"The render target and the depth stencil target are not compatible.",
		"The texture unit ID is invalid.",
		"The texture offset is invalid.",
		"The resource has already been mapped.",
		"The resource is not mapped.",
		"A texture unit was specified, but no texture has been set.",
		"A texture unit was specified, but no texture coordinates have been set.",
		"The shader uniform name is invalid.",
		"The shader uniform is invalid.",
		"The shader signature is invalid.",
		"The shader is invalid.",
		"Currently, the geometry shader cannot use immediate draw.",
		"The command list is not bound.",
		"The bound command list is invalid.",
		"A GetError error occurred in the function. (The error might have occurred due to a previous nngx or gd function call.)",
		"There is no data to save.",
		"The save process has not started.",
		"The save process has already started.",
		"The packet ID is invalid.",
		"All requested insertions in the request list are incompatible with jump recording.",
	};
	if (desc >= 0 && desc <= 38) return map[desc];
	return NULL;
}

static const char* get_desc_str_hio(int desc) {
	const char* const map[] = {
		"InvalidSelection",
		"TooLarge",
		"NotAuthorized",
		"AlreadyDone",
		"InvalidSize",
		"InvalidEnumValue",
		"InvalidCombination",
		"NoData",
		"Busy",
		"MisalignedAddress",
		"MisalignedSize",
		"OutOfMemory",
		"NotImplemented",
		"InvalidAddress",
		"InvalidPointer",
		"InvalidHandle",
		"NotInitialized",
		"AlreadyInitialized",
		"NotFound",
		"CancelRequested",
		"AlreadyExists",
		"OutOfRange",
		"Timeout",
		"InvalidResultValue",
	};
	if (desc >= 0 && desc <= 23) return map[desc];
	return NULL;
}

static const char* get_desc_desc_hio(int desc) {
	const char* const map[] = {
		"An invalid selection was specified.",
		"The size is too large.",
		"Not connected.",
		"Already connected.",
		"The size specification is invalid.",
		"Invalid member value.",
		"Invalid combination of parameters.",
		"There is no data.",
		"WAIT state.",
		"Invalid address alignment.",
		"Invalid size alignment.",
		"Out of memory.",
		"Not implemented.",
		"Invalid address.",
		"Invalid pointer.",
		"Invalid handle.",
		"Not initialized.",
		"Already initialized.",
		"Handle does not exist.",
		"The request was canceled.",
		"Already exists.",
		"Out of range.",
		"Timeout.",
		"Invalid result value.",
	};
	if (desc >= 0 && desc <= 23) return map[desc];
	return NULL;
}

static const char* get_desc_str_http(int desc) {
	const char* const map_00[] = {
		"httpDescription",
		"InvalidStatus",
		"InvalidParam",
		"NoMem",
		"CreateEvent",
		"CreateMutex",
		"CreateQueue",
		"CreateThread",
	};
	const char* const map_10[] = {
		"MsgqSendLsn",
		"MsgqRecvLsn",
		"MsgqRecvComm",
	};
	const char* const map_20[] = {
		"ConnNoMore",
		"ConnNoSuch",
		"ConnStatus",
		"ConnAdd",
		"ConnCanceled",
		"ConnHostMax",
		"ConnProcessMax",
	};
	const char* const map_30[] = {
		"ReqUrl",
		"ReqConnMsgPort",
		"ReqUnknownMethod",
	};
	const char* const map_40[] = {
		"ResHeader",
		"ResNoNewline",
		"ResBodyBuf",
		"ResBodyBufShortage",
	};
	const char* const map_50[] = {
		"PostAddedAnother",
		"PostBoundary",
		"PostSend",
		"PostUnknownEncType",
		"PostNoData",
	};
	const char* const map_60[] = {
		"Ssl",
		"SslCertExist",
		"SslCrlExist",
	};
	const char* const map_70[] = {
		"SocDns",
		"SocSend",
		"SocRecv",
		"SocConn",
		"SocKeepAliveDisconnected",
	};
	const char* const map_100[] = {
		"ConnectionNotInitialized",
		"AlreadyAssignHost",
		"Session",
		"ClientProcessMax",
		"IpcSessionMax",
		"Timeout",
	};
	const char* const map_200[] = {
		"SslNoCaCertStore",
		"SslNoClientCert",
		"SslCaCertStoreMax",
		"SslClientCertMax",
		"SslFailToCreateCertStore",
		"SslFailToCreateClientCert",
		"SslNoCrlStore",
		"SslCrlStoreMax",
		"SslFailToCreateCrlStore",
	};
	if (desc >= 0 && desc <= 7) return map_00[desc];
	if (desc >= 10 && desc <= 12) return map_10[desc - 10];
	if (desc >= 20 && desc <= 26) return map_20[desc - 20];
	if (desc >= 30 && desc <= 32) return map_30[desc - 30];
	if (desc >= 40 && desc <= 43) return map_40[desc - 40];
	if (desc >= 50 && desc <= 54) return map_50[desc - 50];
	if (desc >= 60 && desc <= 62) return map_60[desc - 60];
	if (desc >= 70 && desc <= 74) return map_70[desc - 70];
	if (desc == 80) return "GetProxySetting";
	if (desc >= 100 && desc <= 105) return map_100[desc - 100];
	if (desc >= 200 && desc <= 208) return map_200[desc - 200];
	return NULL;
}

static const char* get_desc_desc_http(int desc) {
	const char* const map_00[] = {
		"No error",
		"Invalid status",
		"Illegal parameter",
		"Dynamic memory allocation failed",
		"Event generation failure",
		"Mutex creation failure",
		"Message queue creation failure",
		"Thread creation failure",
	};
	const char* const map_10[] = {
		"Failure sending to the listener thread message queue.",
		"Failure receiving from the listener thread message queue.",
		"Failure receiving from the communication thread message queue.",
	};
	const char* const map_20[] = {
		"The maximum number of registerable connection handles was exceeded.",
		"No such connection handle.",
		"The connection handle status is invalid.",
		"Connection handle registration failed.",
		"The connection handle was canceled.",
		"Exceeded the maximum number of simultaneous connections to the same host.",
		"Exceeded the maximum number of simultaneous connections used by one process.",
	};
	const char* const map_30[] = {
		"Invalid URL.",
		"Invalid CONNECT send port number.",
		"Unknown method.",
	};
	const char* const map_40[] = {
		"Invalid HTTP header.",
		"No newline character in the HTTP header.",
		"HTTP body receive buffer error.",
		"The HTTP body receive buffer is too small.",
	};
	const char* const map_50[] = {
		"Failure adding to POST data. (A different POST type already exists.)",
		"Boundary cannot be set properly.",
		"POST request send failure.",
		"Unknown encoding type.",
		"Send data not set.",	};
	const char* const map_60[] = {
		"SSL error.",
		"The SSL certificate is already set. (It must be deleted before re-registering.)",
		"The SSL CRL is already set. (It must be deleted before re-registering.)",
	};
	const char* const map_70[] = {
		"DNS name resolution failure.",
		"Socket data send failure.",
		"Socket data receive failure.",
		"Socket connection failure.",
		"Keep-alive communications have been disconnected from the server side.",	};
	const char* const map_100[] = {
		"Destination no allocated",
		"Destination already allocated",
		"The IPC session is invalid",
		"A number of clients equivalent to the maximum number of simultaneous client processes is already being used.",
		"The maximum number of simultaneous IPC session connections is already connected. (The number of clients and number of connections are both already at the maximum.)",
		"Timeout",
	};
	const char* const map_200[] = {
		"No such SSL CA certificate store is registered.",
		"No such SSL client certificate is registered.",
		"The maximum number of simultaneously registerable SSL per-process CA certificate stores is already registered.",
		"The maximum number of simultaneously registerable SSL per-process client certificates is already registered.",
		"Failed to create SSL certificate store.",
		"Failed to create SSL client certificate.",
		"No such SSL CRL store is registered.",
		"The maximum number of simultaneously registerable SSL per-process CRL stores is already registered.",
		"Failed to create SSL CRL store.",
	};
	if (desc >= 0 && desc <= 7) return map_00[desc];
	if (desc >= 10 && desc <= 12) return map_10[desc - 10];
	if (desc >= 20 && desc <= 26) return map_20[desc - 20];
	if (desc >= 30 && desc <= 32) return map_30[desc - 30];
	if (desc >= 40 && desc <= 43) return map_40[desc - 40];
	if (desc >= 50 && desc <= 54) return map_50[desc - 50];
	if (desc >= 60 && desc <= 62) return map_60[desc - 60];
	if (desc >= 70 && desc <= 74) return map_70[desc - 70];
	if (desc == 80) return "Failed to get the proxy value set by the device.";
	if (desc >= 100 && desc <= 105) return map_100[desc - 100];
	if (desc >= 200 && desc <= 208) return map_200[desc - 200];
	return NULL;
}

static const char* get_desc_str_ir(int desc) {
	const char* const map[] = {
		"MachineSleep",
		"FatalError",
		"SignatureNotFound",
		"DifferentSessionID",
		"InvalidCRC",
		"FollowingDataNotExist",
		"FramingError",
		"OverrunError",
		"PerformanceError",
		"ModuleOtherError",
		"AlreadyConnected",
		"AlreadyTryingToConnect",
		"NotConnected",
		"BufferFull",
		"BufferInsufficient",
		"PacketFull",
		"Timeout",
		"Peripheral",
		"PeripheralDataNotExist",
		"CannotConfirmID",
		"InvalidData",
	};
	if (desc >= 1 && desc <= 21) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_ir(int desc) {
	const char* const map[] = {
		"Sleeping because the system is in Sleep Mode.",
		"The IR module may be malfunctioning.",
		"Cannot find a signature indicating the start of the packet.",
		"The session ID is different.",
		"Received data has invalid CRC.",
		"Incomplete data.",
		"The received data has a framing error.",
		"The receiving process could not keep pace with the device's data receive speed.",
		"An error related to system performance or CTR performance at a high baud-rate setting occurred.",
		"Unknown error in the IR module.",
		"Already connected.",
		"Already trying to connect.",
		"Not connected.",
		"The buffer is full.",
		"The buffer is too small.",
		"The packet is full.",
		"A timeout has occurred.",
		"Communication target error.",
		"Data was not found in the communication target.",
		"Could not confirm the ID for communication.",
		"Invalid data was received. (This could be a packet replay attack.)",
	};
	if (desc >= 1 && desc <= 21) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_kern(int desc) {
	const char* const map[] = {
		"NoSuchThread",
		"SessionMustBeLocked",
		"NotBound",
		"NonManualEvent",
		"NonActive",
		"ObjectNotFound",
		"StopProcessingException",
		"InvalidParm",
		"ProcessTerminated",
		"OutOfAddressSpace",
		"NoSyncedObject",
		"NoDebugEvent",
		"OverrideState",
		"TerminateRequested",
		"InconsistencyMemoryBlockRange",
	};
	if (desc >= 512 && desc <= 512 + 14) return map[desc - 512];
	return NULL;
}

static const char* get_desc_desc_kern(int desc) {
	const char* const map[] = {
		"No such thread",
		"Called without locking",
		"Specified value out of range",
		"Improper Clear call",
		"Improper Clear call",
		"Operations on Objects not contained in containers",
		"Stop processing exception",
		"Invalid parameter",
		"The specified process has already terminated",
		"Insufficient contiguous free space in address space",
		"Synchronization object not set",
		"No debug events",
		"Page table state overwritten or taken by others",
		"Thread termination requested",
		"Memory block range is inconsistent",
	};
	if (desc >= 512 && desc <= 512 + 14) return map[desc - 512];
	return NULL;
}

static const char* get_desc_str_l2b(int desc) {
	const char* const map[] = {
		"IsSleeping",
		"InvalidL2bNo",
	};
	if (desc >= 1 && desc <= 2) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_l2b(int desc) {
	const char* const map[] = {
		"The L2B is sleeping.",
		"The L2B number is invalid.",
	};
	if (desc >= 1 && desc <= 2) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_ldr(int desc) {
	const char* const map[] = {
		"FailedHostFileOperation",
		"InvalidRomFormat",
		"NoEntry",
		"OutOfMemory",
		"InvalidProgramLaunchInfo",
	};
	if (desc >= 1 && desc <= 5) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_ldr(int desc) {
	const char* const map[] = {
		"Failed to perform host file operation",
		"Invalid ROM format",
		"Entry function does not exist",
		"Out of memory",
		"Invalid program launch information",
	};
	if (desc >= 1 && desc <= 5) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_mcu(int desc) {
	if (desc == 1) return "InvalidAddressOrScale";
	return NULL;
}

static const char* get_desc_desc_mcu(int desc) {
	if (desc == 1) return "Invalid address or abnormal scale value is specified.";
	return NULL;
}

static const char* get_desc_str_mic(int desc) {
	if (desc == 1) return "MicShellClose";
	return NULL;
}

static const char* get_desc_desc_mic(int desc) {
	if (desc == 1) return "The microphone cannot be used because the system is closed.";
	return NULL;
}

static const char* get_desc_str_midi(int desc) {
	const char* const map[] = {
		"AlreadyOpened",
		"NotOpened",
		"BufferOverflow",
		"DeviceFifoFull",
		"DeviceFrameError",
		"DeviceInvalidDataLength",
		"UnknownDevice",
	};
	if (desc >= 1 && desc <= 7) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_midi(int desc) {
	const char* const map[] = {
		"The device is already opened.",
		"The device is not opened.",
		"The buffer is full.",
		"The device FIFO is full.",
		"The device frame error.",
		"The device invalid data length.",
		"The device is unknown.",
	};
	if (desc >= 1 && desc <= 7) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_mp(int desc) {
	const char* const map[] = {
		"Failed",
		"IllegalState",
		"InvalidParam",
		"NoChild",
		"NoEntry",
		"OverMaxEntry",
		"NoData",
		"NoDataset",
		"NotAllowed",
		"AlreadyInUse",
		"Closed",
		"NotEnoughMemory",
		"NotInitialized",
		"Abort",
		"WirelessOff",
		"Operating",
		"SendQueueFull",
	};
	if (desc >= 1 && desc <= 17) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_mp(int desc) {
	const char* const map[] = {
		"An internal error (WL command error) has occurred.",
		"You called a function that cannot be called in the current state of the MP library.",
		"Parameter is invalid.",
		"The specified child machine does not exist. Only used on the parent machine.",
		"Cannot connect because no entries have been accepted. Only used on child devices.",
		"Cannot connect because the maximum number of devices is being connected. Only used on child devices.",
		"There was no data to process. It happens in the Receive() function.",
		"There was no data sharing data to process. Occurs in the DSTryStep() function.",
		"Not allowed. Returns when Start() is used for a channel that is not allowed to be used.",
		"The specified channel is already in use.",
		"Failed because termination processing has already been performed.",
		"Could not allocate required memory.",
		"The MP library has not been initialized.",
		"Processing interrupted. Returns when calling AbortReceive() or AbortGetIndication() while Receive() or GetIndication() is running.",
		"Failed because the wireless function is off.",
		"Running. (Driver internal error)",
		"Send queue is full. (Driver internal error)",
	};
	if (desc >= 1 && desc <= 17) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_mvd(int desc) {
	const char* const map1[] = {
		"Ok",
		"StrmProcessed",
		"PicRdy",
		"PicDecoded",
		"HdrsRdy",
		"AdvancedTools",
		"PendingFlush",
		"NonrefPicSkipped",
	};
	const char* const map2[] = {
		"LibsOffset",
		"MemNotInitialized",
		"MemAlreadyInitialized",
		"InputMallocFail",
		"DoneNothing",
	};
	const char* const map3[] = {
		"DriverErrorOffset1",
		"ParamError",
		"StrmError",
		"NotInitialized",
		"Memfail",
		"Initfail",
		"HdrsNotRdy",
		"StreamNotSupported",
	};
	const char* const map4[] = {
		"PpSetInSizeInvalid",
		"PpSetInAddressInvalid",
		"PpSetInFormatInvalid",
		"PpSetCropInvalid",
		"PpSetRotationInvalid",
		"PpSetOutSizeInvalid",
		"PpSetOutAddressInvalid",
		"PpSetOutFormatInvalid",
		"PpSetVideoAdjustInvalid",
		"PpSetRgbBitmaskInvalid",
		"PpSetFramebufferInvalid",
		"PpSetMask1Invalid",
		"PpSetMask2Invalid",
		"PpSetDeinterlaceInvalid",
		"PpSetInStructInvalid",
		"PpSetInRangeMapInvalid",
		"PpSetAblendUnsupported",
		"PpSetDeinterlacingUnsupported",
		"PpSetDitheringUnsupported",
		"PpSetScalingUnsupported",
	};
	const char* const map5[] = {
		"HwReserved",
		"HwTimeout",
		"HwBusError",
		"SystemError",
		"DwlError",
	};
	if (desc >= 0 && desc <= 7) return map1[desc];
	if (desc == 50) return "DriverSucessOffset";
	if (desc == 56) return "SliceRdy";
	if (desc >= 100 && desc <= 104) return map2[desc - 100];
	if (desc >= 200 && desc <= 207) return map3[desc - 200];
	if (desc >= 264 && desc <= 264 + 19) return map4[desc - 264];
	if (desc == 328) return "PpBusy";
	if (desc >= 454 && desc <= 454 + 4) return map5[desc - 454];
	if (desc == 712) return "PpDecCombinedModeError";
	if (desc == 713) return "PpDecRuntimeError";
	if (desc == 800) return "DriverErrorOffset2";
	if (desc == 899) return "EvaluationLimitExceeded";
	if (desc == 900) return "FormatNotSupported";
	if (desc == 1000) return "SdkReserved";
	return NULL;
}

static const char* get_desc_desc_mvd(int desc) {
	const char* const map1[] = {
		"No error",
		"Stream buffer processed",
		"Picture ready",
		"Picture decoded",
		"Headers decoded",
		"Advanced coding tools detected in stream",
		"HW is busy but no more stream",
		"Non-reference picture skipped in decoding",
	};
	const char* const map2[] = {
		"LibsOffset",
		"Memory not initialized",
		"Memory already initialized",
		"Input allocation failed",
		"Nothing was done",
	};
	const char* const map3[] = {
		"DriverErrorOffset1",
		"Invalid parameters",
		"Invalid stream structure",
		"Trying to do HW init without HW being initialized",
		"Memory allocation failed",
		"HW init failed",
		"Headers information is not ready",
		"The stream is not supported",
	};
	const char* const map4[] = {
		"PP: Input size is invalid",
		"PP: Input address is invalid",
		"PP: Input format is invalid",
		"PP: Cropping size is invalid",
		"PP: Rotation is invalid",
		"PP: Output size is invalid",
		"PP: Output address is invalid",
		"PP: Output format is invalid",
		"PP: Video adjustment is invalid",
		"PP: RGB bit mask is invalid",
		"PP: Frame buffer is invalid",
		"PP: Mask 1 is invalid",
		"PP: Mask 2 is invalid",
		"PP: Deinterlacing setting is invalid",
		"PP: Input structure is invalid",
		"PP: Input range mapping is invalid",
		"PP: Alpha blending is not supported",
		"PP: Deinterlacing is not supported",
		"PP: Dithering is not supported",
		"PP: Scaling is not supported",
	};
	const char* const map5[] = {
		"HW: Reserved value",
		"HW: Timeout occurred",
		"HW: A bus error occurred",
		"System: A system error occurred",
		"DWL: A DWL error occurred",
	};
	if (desc >= 0 && desc <= 7) return map1[desc];
	if (desc == 50) return "DriverSucessOffset";
	if (desc == 56) return "HW finished processing a slice";
	if (desc >= 100 && desc <= 104) return map2[desc - 100];
	if (desc >= 200 && desc <= 207) return map3[desc - 200];
	if (desc >= 264 && desc <= 264 + 19) return map4[desc - 264];
	if (desc == 328) return "PP: Trying to set PP configuration while PP is busy";
	if (desc >= 454 && desc <= 454 + 4) return map5[desc - 454];
	if (desc == 712) return "PP: Combined mode not allowed";
	if (desc == 713) return "PP: Runtime error occurred";
	if (desc == 800) return "DriverErrorOffset2";
	if (desc == 899) return "Evaluation limit exceeded";
	if (desc == 900) return "Format not supported";
	if (desc == 1000) return "SdkReserved";
	return NULL;
}

static const char* get_desc_str_ndm(int desc) {
	const char* const map[] = {
		"InterruptByRequest",
		"ProcessingPriorityRequest",
		"InErrorState",
		"Disconnected",
		"CancelledByOtherRequest",
		"CancelledByHardwareEvents",
		"CancelledByDisconnect",
		"CancelledByUserRequest",
		"OperationDenied",
		"LockedByOtherProcess",
		"NotLocked",
		"AlreadySuspended",
		"NotSuspended",
		"InvalidOperation",
		"NotExclusive",
		"ExclusiveByOtherProcess",
		"ExclusiveByOwnProcess",
		"BackgroundDisconnected",
		"ForegroundConnectionExists",
	};
	if (desc >= 1 && desc <= 19) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_ndm(int desc) {
	const char* const map[] = {
		"Interrupted by request",
		"Already processing priority request",
		"In error state",
		"The network daemon manager is disconnected",
		"Cancelled by other request",
		"Cancelled by hardware events",
		"Cancelled by disconnect",
		"Cancelled by user request",
		"Operation denied",
		"Locked by other process",
		"Not locked",
		"Already suspended",
		"Not suspended",
		"Invalid operation",
		"Not exclusive",
		"Exclusive by other process",
		"Exclusive by own process",
		"Background disconnected",
		"Foreground connection exists",
	};
	if (desc >= 1 && desc <= 19) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_news(int desc) {
	const char* const map[] = {
		"None",
		"InvalidSubjectSize",
		"InvalidMessageSize",
		"InvalidPictureSize",
		"InvalidPicture",
	};
	if (desc >= 0 && desc <= 4) return map[desc];
	return NULL;
}

static const char* get_desc_desc_news(int desc) {
	const char* const map[] = {
		"No error.",
		"The length of the subject line is too large.",
		"The message body is too large.",
		"The image is too large.",
		"Invalid image.",
	};
	if (desc >= 0 && desc <= 4) return map[desc];
	return NULL;
}

static const char* get_desc_str_nfc(int desc) {
	const char* const map[] = {
		"InvalidOperation",
		"InvalidArgument",
		"TimeoutError",
		"ConnectionError",
		"UnknownError",
		"NotSupported",
		"BufferSmall",
		"UnknownFormat",
		"NoResources",
		"IsBusy",
		"NfcFunctionError",
		"TagNotFound",
		"NeedRetry",
		"NeedFormat",
		"InternalError",
		"HardwareError",
	};
	if (desc >= 1 && desc <= 16) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_nfc(int desc) {
	const char* const map[] = {
		"Invalid operation was requested.",
		"Invalid argument was passed.",
		"Timeout error occurred.",
		"Connection error occurred.",
		"Unknown error occurred.",
		"Not supported.",
		"Buffer is too small.",
		"Unknown format.",
		"Not enough resources.",
		"Device is busy.",
		"NFC function error occurred.",
		"Tag was not found.",
		"Need retry.",
		"Need format.",
		"Internal error occurred.",
		"Hardware error occurred.",
	};
	if (desc >= 1 && desc <= 16) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_nim(int desc) {
	const char* const map[] = {
		"Ok",
		"NotTerminatedString",
		"TooManySessions",
		"UnknownEcError",
		"UnknownState",
		"AlreadyDownloaded",
		"Terminating",
		"BossFailed",
		"UnsupportedHttpStatusCode",
		"HashChanged",
		"OverAgeLimit",
		"NeedAgreeLatestEula",
		"InvalidCountryCode",
		"InvalidSerialNumber",
		"NupCapacityOver",
		"NoTitle",
		"InvalidTitleList",
		"InvalidTitle",
		"InvalidSaveData",
		"Suspending",
		"InvalidCombination",
		"InvalidTitleVersion",
		"InvalidData",
		"CannotSetIvs",
		"AcNotConnected",
		"NeedGetIvs",
		"CannotGetIvs",
		"InvalidLanguageCode",
		"InvalidDownloadType",
		"NotPrepared",
		"NotSupported",
		"TitleNotDownloaded",
		"OutOfFilterMemory",
		"OutOfDownloadTaskList",
		"BufferNotEnough",
		"AccountNotCreated",
		"OutOfCatalogMemory",
		"StandbyMode",
	};
	if (desc >= 0 && desc <= 37) return map[desc];
	switch (desc) {
		case 100: return "EcErrorOk";
		case 101: return "EcErrorFail";
		case 102: return "EcErrorNotSupported";
		case 104: return "EcErrorInvalid";
		case 105: return "EcErrorNomem";
		case 106: return "EcErrorNotFound";
		case 107: return "EcErrorNotBusy";
		case 108: return "EcErrorBusy";
		case 110: return "EcErrorOutmem";
		case 115: return "EcErrorWsReport";
		case 117: return "EcErrorEcard";
		case 118: return "EcErrorOverflow";
		case 119: return "EcErrorNetContent";
		case 132: return "EcErrorMinReplenish";
		case 133: return "EcErrorMaxBalance";
		case 134: return "EcErrorWsResp";
		case 138: return "EcErrorCanceled";
		case 139: return "EcErrorAlready";
		case 140: return "EcErrorTimeout";
		case 141: return "EcErrorInit";
		case 143: return "EcErrorWsRecv";
		case 149: return "EcErrorNeedUpdate";
		case 153: return "EcErrorConfig";
		case 159: return "EcErrorBalanceNotCleared";
		case 160: return "EcErrorStreamWrite";
		case 161: return "EcErrorStreamWriteSize";
		case 164: return "EcErrorEciBusy";
		case 167: return "EcErrorEciStandby";
		case 168: return "EcErrorInvalidToken";
		case 169: return "EcErrorConnect";
		case 173: return "EcErrorMigrateLimitReached";
		case 177: return "EcErrorAgeRestricted";
		case 179: return "EcErrorAlreadyOwn";
		case 181: return "EcErrorInsufficientFunds";
		case 299: return "EcErrorResultModule";
	}
	return NULL;
}

static const char* get_desc_desc_nim(int desc) {
	const char* const map[] = {
		"Success.",
		"The string is not terminated.",
		"Too many sessions.",
		"Unknown EC error.",
		"Unknown state.",
		"The title is already downloaded.",
		"Terminating.",
		"Background Online System Service failed.",
		"Unsupported HTTP status code.",
		"The hash value has changed.",
		"The user is over the age limit.",
		"The user needs to agree to the latest EULA.",
		"The country code is invalid.",
		"The serial number is invalid.",
		"The NUP capacity is over.",
		"The title does not exist.",
		"The title list is invalid.",
		"The title is invalid.",
		"The save data is invalid.",
		"Suspending.",
		"The combination is invalid.",
		"The title version is invalid.",
		"The data is invalid.",
		"Cannot set IVS.",
		"The AC is not connected.",
		"Need to get IVS.",
		"Cannot get IVS.",
		"The language code is invalid.",
		"The download type is invalid.",
		"Not prepared.",
		"Not supported.",
		"The title is not downloaded.",
		"Out of filter memory.",
		"Out of download task list.",
		"The buffer is not enough.",
		"The account is not created.",
		"Out of catalog memory.",
		"Standby mode.",
	};
	if (desc >= 0 && desc <= 37) return map[desc];
	switch (desc) {
		case 100: return "Success.";
		case 101: return "Fail.";
		case 102: return "Not supported.";
		case 104: return "Invalid.";
		case 105: return "No memory.";
		case 106: return "Not found.";
		case 107: return "Not busy.";
		case 108: return "Busy.";
		case 110: return "Out of memory.";
		case 115: return "WS report.";
		case 117: return "E-card.";
		case 118: return "Overflow.";
		case 119: return "Net content.";
		case 132: return "Minimum replenish.";
		case 133: return "Maximum balance.";
		case 134: return "Ws response.";
		case 138: return "Canceled.";
		case 139: return "Already.";
		case 140: return "Timeout.";
		case 141: return "Init.";
		case 143: return "WS receive.";
		case 149: return "Need update.";
		case 153: return "Config.";
		case 159: return "Balance not cleared.";
		case 160: return "Stream write.";
		case 161: return "Stream write size.";
		case 164: return "ECI busy.";
		case 167: return "ECI standby.";
		case 168: return "Invalid token.";
		case 169: return "Connect.";
		case 173: return "Migrate limit reached.";
		case 177: return "Age restricted.";
		case 179: return "Already own.";
		case 181: return "Insufficient funds.";
		case 299: return "Result module.";
	}
	return NULL;
}

static const char* get_desc_str_ns(int desc) {
	const char* const map[] = {
		"RebootNotRequired",
		"ShutdownProcessing",
		"DemoLaunchLimited",
		"ContentNotFound",
	};
	if (desc >= 1 && desc <= 4) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_ns(int desc) {
	const char* const map[] = {
		"Reboot is not required.",
		"Shutting down.",
		"Demo launch is limited.",
		"Content not found.",
	};
	if (desc >= 1 && desc <= 4) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_nwm(int desc) {
	const char* const map[] = {
		"SdioInitFailure",
		"ModuleInitFailure",
		"ModuleEventFailure",
		"OtherFirmwareError",
		"CommandFailure",
		"InvalidState",
		"StateMismatch",
		"DuplicateRxEntry",
		"RxEntryNotFound",
		"BssNotFound",
		"LinkingError",
		"WifiOff",
		"AlreadyInState",
		"Operating",
		"InfraFirmwareError",
		"SapFirmwareError",
		"CecFirmwareError",
	};
	if (desc >= 1 && desc <= 17) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_nwm(int desc) {
	const char* const map[] = {
		"Failed to initialize SDIO",
		"Failed to initialize the wireless module.",
		"An illegal event has occurred from the wireless module.",
		"Radio module firmware error on non-Infra/SoftAP/CEC",
		"Command operation to the wireless module failed.",
		"Error due to state inconsistency.",
		"The requested state and final transition destination state do not match.",
		"Error in radio reception entry registration.",
		"Error deleting radio receive entry.",
		"Destination BSS was not found.",
		"Error during connection sequence.",
		"An attempt was made to perform a wireless operation with WiFi turned off.",
		"Already in the desired state.",
		"API is running. (for Scan/Measure Channel)",
		"Infra radio module firmware error",
		"SoftAP radio module firmware error",
		"CEC radio module firmware error",
	};
	if (desc >= 1 && desc <= 17) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_os(int desc) {
	const char* const map[] = {
		"FailedToAllocateMemory",
		"FailedToAllocateSharedMemory",
		"FailedToAllocateThread",
		"FailedToAllocateMutex",
		"FailedToAllocateSemaphore",
		"FailedToAllocateEvent",
		"FailedToAllocateTimer",
		"FailedToAllocatePort",
		"FailedToAllocateSession",
		"ExceedMemoryLimit",
		"ExceedSharedMemoryLimit",
		"ExceedThreadLimit",
		"ExceedMutexLimit",
		"ExceedSemaphoreLimit",
		"ExceedEventLimit",
		"ExceedTimerLimit",
		"ExceedPortLimit",
		"ExceedSessionLimit",
		"MaxHandle",
		"InnaccessiblePage",
		"Abandoned",
		"Abandon1",
		"Abandon2",
		"InvalidProcessId",
		"InvalidThreadId",
		"SessionClosed",
		NULL,
		"InvalidMessage",
		"ManualResetEventRequired",
		"TooLongName",
		"NotOwned",
		"ProcessTerminated",
		"InvalidTlsIndex",
		"NoRunnableProcessor",
		"NoSession",
		"UsingRegion",
		"AlreadyReceived",
		"CancelRequested",
		"NotReceived",
		"Abandoned3",
		"DeliverArgNotReady",
		"DeliverArgOverSize",
		"InvalidDeliverArg",
		"IAmOwner",
		"ExceedsSharedLimit",
		"UnexpectedPermission",
		"InvalidTag",
		"InvalidFormat",
		"OtherHandle",
		"FailedToAllocateAddressArbiter",
		"ExceedAddressArbiterLimit",
		"OverPortCapacity",
		"NotMapped",
		"BufferTooFlagmented",
		"NoAddressSpace",
		"ExceedTlsLimit",
		"CantStart",
		"Locked",
		"NotFinalized",
	};
	if (desc >= 1 && desc <= 59) return map[desc - 1];
	if (desc == 1023) return "ObsoleteResult";
	return NULL;
}

static const char* get_desc_desc_os(int desc) {
	const char* const map[] = {
		"Reached the physical memory limit",
		"Reached the shared memory limit",
		"Reached the thread limit",
		"Reached the mutex limit",
		"Reached the semaphore limit",
		"Reached the event limit",
		"Reached the timer limit",
		"Reached the port limit",
		"Reached the session limit",
		"Reached the physical memory allocation limit",
		"Reached the shared memory allocation limit",
		"Reached the thread allocation limit",
		"Reached the mutex allocation limit",
		"Reached the semaphore allocation limit",
		"Reached the event allocation limit",
		"Reached the timer allocation limit",
		"Reached the port allocation limit",
		"Reached the session allocation limit",
		"Reached the maximum number of handles",
		"An inaccessible page is included",
		"An object was abandoned",
		"No longer used",
		"No longer used",
		"Invalid process ID",
		"Invalid thread ID",
		"The session was closed",
		NULL,
		"Invalid message",
		"A manual reset event is required",
		"The name is too long",
		"The mutex is not owned",
		"The specified process already terminated",
		"An unallocated TLS index was specified",
		"Affinity mask prohibits running on all processors",
		"Not a newly connected session",
		"Region in use",
		"Already received",
		"Cancel requested",
		"Not received",
		"No longer used",
		"DeliverArg not ready",
		"DeliverArg over size",
		"Invalid DeliverArg",
		"I am owner",
		"Exceeds shared limit",
		"Unexpected permission",
		"Invalid IPC message tag",
		"Invalid format",
		"Closed by other handle",
		"Failed to allocate address arbiter",
		"Reached the address arbiter limit",
		"Over IPC port capacity",
		"Not mapped",
		"Buffer too fragmented",
		"No address space",
		"Reached the TLS limit",
		"Canceled",
		"Locked",
		"Not finalized",
	};
	if (desc >= 1 && desc <= 59) return map[desc - 1];
	if (desc == 1023) return "Obsolete; this error must be changed to another error as soon as possible.";
	return NULL;
}

static const char* get_desc_str_pdn(int desc) {
	if (desc == 1) return "ClockNotSupplied";
	return NULL;
}

static const char* get_desc_desc_pdn(int desc) {
	if (desc == 1) return "Clock not supplied";
	return NULL;
}

static const char* get_desc_str_pl(int desc) {
	if (desc == 2) return "SharedfontNotFound";
	if (desc == 100) return "GamecoinDataReset";
	if (desc == 101) return "LackOfGamecoin";
	return NULL;
}

static const char* get_desc_desc_pl(int desc) {
	if (desc == 2) return "The shared font was not found.";
	if (desc == 100) return "The data was processed successfully, but was reset because there was a problem in the save data.";
	if (desc == 101) return "The user attempted to use more game coins than they own.";
	return NULL;
}

static const char* get_desc_str_pmlow(int desc) {
	const char* const map[] = {
		"InvalidRomFormat",
		"FailedToReadProgramInfo",
		"ProgramNotLoaded",
		"FailedToVerifyAcidSignature",
		"InvalidAcid",
	};
	if (desc >= 1 && desc <= 5) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_pmlow(int desc) {
	const char* const map[] = {
		"Invalid ROM format",
		"Failed to read program info",
		"Program not loaded",
		"Failed to verify access control info descriptor signature",
		"Invalid access control info descriptor",
	};
	if (desc >= 1 && desc <= 5) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_ptm(int desc) {
	const char* const map[] = {
		"InvalidSystemtime",
		"Noalarm",
		"Overwritealarm",
		"Fileerror",
		"NotSleeping",
		"InvalidTrigger",
		"McuFatalError",
	};
	if (desc >= 1 && desc <= 7) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_ptm(int desc) {
	const char* const map[] = {
		"The system time is invalid.",
		"Indicates that the alarm has not been configured.",
		"Indicates that the alarm has already been configured.",
		"Indicates that an error occurred while accessing the file.",
		"Indicates that the system is not in sleep mode.",
		"Indicates that the trigger is invalid.",
		"Indicates that a fatal error occurred in the MCU.",
	};
	if (desc >= 1 && desc <= 7) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_qtm(int desc) {
	const char* const map[] = {
		"FatalError",
		"InvalidArgument",
		"IsSleeping",
		"InvalidMemoryAllocation",
		"IrLedInvalidState",
		"CameraInvalidState",
		"InvalidLuminance",
		"CameraExclusive",
		"NotAvailable",
	};
	if (desc >= 1 && desc <= 9) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_qtm(int desc) {
	const char* const map[] = {
		"A fatal error has occurred.",
		"An invalid argument was passed.",
		"The device is sleeping.",
		"Memory allocation failed.",
		"The IR LED is in an invalid state.",
		"The camera is in an invalid state.",
		"The luminance is invalid.",
		"The camera is in use by another process.",
		"The requested function is not available.",
	};
	if (desc >= 1 && desc <= 9) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_rdt(int desc) {
	const char* const map[] = {
		"ResetReceived",
		"UntimelyCall",
		"InvalidValue",
	};
	if (desc >= 1 && desc <= 3) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_rdt(int desc) {
	const char* const map[] = {
		"Thesese results transition to a CLOSED state because a reset signal was received from the partner.",
		"Do not call the function in this state.",
		"Invalid parameter value.",
	};
	if (desc >= 1 && desc <= 3) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_ro(int desc) {
	const char* const map[] = {
		"AlreadyLoaded",
		"AtexitNotFound",
		"BrokenObject",
		"ControlObjectNotFound",
		"EitNodeNotFound",
		"InvalidList",
		"InvalidOffsetRange",
		"InvalidRegion",
		"InvalidSign",
		"InvalidSign2",
		"InvalidString",
		"InvalidTarget",
		"NotLoaded",
		"NotRegistered",
		"OutOfSpace",
		"RangeErrorAtExport",
		"RangeErrorAtHeader",
		"RangeErrorAtImport",
		"RangeErrorAtIndexExport",
		"RangeErrorAtIndexImport",
		"RangeErrorAtInternalRelocation",
		"RangeErrorAtOffsetExport",
		"RangeErrorAtOffsetImport",
		"RangeErrorAtReference",
		"RangeErrorAtSection",
		"RangeErrorAtSymbolExport",
		"RangeErrorAtSymbolImport",
		"RegistrationNotFound",
		"RwNotSupported",
		"StaticModule",
		"TooSmallSize",
		"TooSmallTarget",
		"UnknownObjectControl",
		"UnknownRelocationType",
		"VeneerRequired",
		"VerificationFailed",
	};
	if (desc >= 1 && desc <= 36) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_ro(int desc) {
	const char* const map[] = {
		"The module is already loaded.",
		"The atexit function is not found.",
		"The module is broken.",
		"The control object is not found.",
		"The EIT node is not found.",
		"The list is invalid.",
		"The offset range is invalid.",
		"The region is invalid.",
		"The sign is invalid.",
		"The sign2 is invalid.",
		"The string is invalid.",
		"The target is invalid.",
		"The module is not loaded.",
		"The module is not registered.",
		"There is no space.",
		"The range error occurs at export.",
		"The range error occurs at header.",
		"The range error occurs at import.",
		"The range error occurs at index export.",
		"The range error occurs at index import.",
		"The range error occurs at internal relocation.",
		"The range error occurs at offset export.",
		"The range error occurs at offset import.",
		"The range error occurs at reference.",
		"The range error occurs at section.",
		"The range error occurs at symbol export.",
		"The range error occurs at symbol import.",
		"The registration is not found.",
		"The RW is not supported.",
		"The module is static.",
		"The size is too small.",
		"The target is too small.",
		"The object control is unknown.",
		"The relocation type is unknown.",
		"The veneer is required.",
		"The verification failed.",
	};
	if (desc >= 1 && desc <= 36) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_snd(int desc) {
	if (desc == 1) return "SndNoDspComponentLoaded";
	return NULL;
}

static const char* get_desc_desc_snd(int desc) {
	if (desc == 1) return "No DSP component is loaded.";
	return NULL;
}

static const char* get_desc_str_socket(int desc) {
	const char* const map[] = {
		"FailedToInitializeInterface",
		"FailedToInitializeSocketCore",
		"TooManyRequests",
		"PermissionDenied",
		"UnkownConfiguration",
		"UnkownClient",
		"BadDescriptor",
		"RequestSessionFull",
		"NetworkResetted",
		"TooManyProcesses",
		"AlreadyRegistered",
		"TooManySockets",
		"MismatchVersion",
		"AddressCollision",
		"Timeout",
		"OutOfSystemResource",
		"InvalidCoreState",
		"Aborted",
	};
	if (desc >= 1 && desc <= 18) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_socket(int desc) {
	const char* const map[] = {
		"Failed to initialize socket interface",
		"Failed to initialize socket core",
		"Too many requests",
		"Permission denied",
		"Unknown configuration",
		"Unknown client",
		"Bad descriptor",
		"Request session full",
		"Network resetted",
		"Too many processes",
		"Already registered",
		"Too many sockets",
		"Mismatch version",
		"Address collision",
		"Timeout",
		"Out of system resource",
		"Invalid core state",
		"Aborted",
	};
	if (desc >= 1 && desc <= 18) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_srv(int desc) {
	const char* const map[] = {
		"FailedSynchronization",
		"NoSuchHandle",
		"AlreadyExists",
		"NotExists",
		"TooLongServiceName",
		"NotPermitted",
		"InvalidName",
		"BufferOverflow",
	};
	if (desc >= 1 && desc <= 8) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_srv(int desc) {
	const char* const map[] = {
		"Failed to synchronize with the server",
		"No such handle exists",
		"The handle already exists",
		"The handle does not exist",
		"The service name is too long",
		"The operation is not permitted",
		"The service name is invalid",
		"The buffer is too big",
	};
	if (desc >= 1 && desc <= 8) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_ssl(int desc) {
	const char* const map1[] = {
		"None",
		"Failed",
		"WantRead",
		"WantWrite",
		NULL,
		"SysCall",
		"ZeroReturn",
		"WantConnect",
		"SslId",
		"VerifyCommonName",
		"VerifyRootCa",
		"VerifyChain",
		"VerifyDate",
		"GetServerCert",
		NULL, NULL,
		"VerifyRevokedCert",
		"State",
		NULL,
		"Random",
		"VerifyCert",
		"CertBufAlreadySet",
	};
	const char* const map2[] = {
		"NotAssignServer",
		"AlreadyAssignServer",
		"IpcSession",
		"ConnProcessMax",
		"FailToCreateCertStore",
		"FailToCreateCrlStore",
		"FailToCreateClientCert",
		"InvalidParam",
		"ClientProcessMax",
		"IpcSessionMax",
		"InternalCert",
		"InternalCrl",
	};
	if (desc >= 0 && desc <= 21) return map1[desc];
	if (desc >= 50 && desc <= 50 + 11) return map2[desc - 50];
	return NULL;
}

static const char* get_desc_desc_ssl(int desc) {
	const char* const map1[] = {
		"No error.",
		"Error due to SSL protocol failure. (If verification of the client certificate fails on the server side, etc.)",
		"Processing of the Read function is not completed when using an asynchronous socket. (Please try again.)",
		"Processing of the Write function is not completed when using an asynchronous socket. (Please try again.)",
		NULL,
		"An internal system function returned an unexpected error.",
		"Zero was returned at an unexpected timing when Socket Read/Write was executed internally.",
		"SSL handshake (DoHandshake) not completed when using asynchronous socket. (Please try again.)",
		"Internal error (bad SSLID)",
		"Server authentication error. The CommonName of the server certificate does not match the host name of the destination server specified in AssignServer().",
		"Server authentication error. The Root CA certificate of the server certificate does not match the certificate configured for Connection.",
		"Server authentication error. The certificate chain of the server certificate is invalid.",
		"Server authentication error. Server certificate is out of date.",
		"Failed to store certificate data in buffer (Occurs when certificate size is larger than buffer in DoHandshake() with arguments)",
		NULL, NULL,
		"Server authentication error. The server certificate was registered in the revocation list.",
		"The status of the SSL library is incorrect (occurs when \"another library function was executed without initializing\").",
		NULL,
		"Random number processing error.",
		"Server certificate verification NG.",
		"A buffer for storing the server certificate has already been set.",
	};
	const char* const map2[] = {
		"Destination server not assigned",
		"Destination server already assigned",
		"Bad IPC session",
		"Exceeded the maximum number of connections used by one process",
		"Failed to create certificate store",
		"Failed to create CRL store",
		"Failed to create client certificate",
		"Invalid parameter",
		"The number of clients that can be used simultaneously is already in use.",
		"The number of IPC sessions that can be connected at the same time has already been connected (that is, both the number of clients and the number of connections have reached their maximum values).",
		"Failed to use built-in certificate",
		"Failed to use built-in CRL",
	};
	if (desc >= 0 && desc <= 21) return map1[desc];
	if (desc >= 50 && desc <= 50 + 11) return map2[desc - 50];
	return NULL;
}

static const char* get_desc_str_tcb(int desc) {
	const char* const map[] = {
		"UsingRegion",
		"FailedToAllocateCodeset",
		"FailedToAllocateProcess",
		"FailedToAllocateResourceLimit",
		"ExceedCodesetLimit",
		"ExceedProcessLimit",
		"ExceedResourceLimit",
		"ExceedMemoryLimit",
		"ExceedThreadLimit",
		"FailedToAllocateMemory",
		"InaccessiblePage",
		"MaxHandle",
		"FailedToAllocateThread",
		"InterruptNumberAlreadyPermitted",
		"SvcNumberAlreadyPermitted",
		"StaticPageAlreadyMapped",
		"IoPageAlreadyMapped",
		"WrongCapabilityFlag",
		"RequireNewSystem",
		"ApplicationAssigned",
	};
	if (desc >= 1 && desc <= 20) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_tcb(int desc) {
	const char* const map[] = {
		"UsingRegion",
		"Failed to allocate codeset",
		"Failed to allocate process",
		"Failed to allocate resource limit",
		"Codeset quota limit reached",
		"Process quota reached",
		"Resource limit quota reached",
		"Physical memory allocation limit reached",
		"Thread allocation limit reached",
		"Failed to allocate physical memory",
		"Contains inaccessible pages",
		"Maximum number of handles reached",
		"Failed to allocate thread",
		"Interrupt number accepted",
		"SVC number accepted",
		"Static page is mapped",
		"I/O page is already mapped",
		"Wrong capability flag",
		"Require new system",
		"Application assigned",
	};
	if (desc >= 1 && desc <= 20) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_uds(int desc) {
	const char* const map[] = {
		"NetworkIsFull",
		"WifiOff",
		"InvalidParams",
		"MiscellaneousSystemError",
		"MalformedData",
		"InvalidSdkVersion",
		"SystemError",
		"InvalidData",
	};
	if (desc >= 1 && desc <= 8) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_uds(int desc) {
	const char* const map[] = {
		"Could not connect because the maximum number of stations that can connect to the network has been reached.",
		"Failed because the system is in wireless off mode.",
		"A parameter error other than OutOfRange, TooLarge, or NotAuthorized.",
		"The function failed because of a system error. It may succeed if called again.",
		"Detected the possibility of tampering.",
		"Non-public error: the UDS SDK version is invalid.",
		"The function failed because of a system error.",
		"Detected invalid data.",
	};
	if (desc >= 1 && desc <= 8) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_updater(int desc) {
	const char* const map[] = {
		"NotExpectedVersion",
		"HashMismatch",
		"NotExpectedMcuFirmVersion",
		"FailedMcuFirmUpdate",
		"FailedAllocateBuffer",
		"InvalidHashLength",
		"FailedReadContent",
		"NotSupported",
		"FailedUpdateMcu",
	};
	if (desc >= 1 && desc <= 9) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_updater(int desc) {
	const char* const map[] = {
		"The version of the firmware is not as expected.",
		"The hash of the firmware is not as expected.",
		"The version of the MCU firmware is not as expected.",
		"Failed to update the MCU firmware.",
		"Failed to allocate buffer.",
		"The length of the hash is invalid.",
		"Failed to read content.",
		"The operation is not supported.",
		"Failed to update the MCU.",	};
	if (desc >= 1 && desc <= 9) return map[desc - 1];
	return NULL;
}

static const char* get_desc_str_vctl(int desc) {
	const char* const map[] = {
		"NotInitialized",
		"AlreadyInitialized",
		"UnsupportedVersion",
		"InvalidArgument",
		"NotEnoughSpace",
	};
	if (desc >= 1 && desc <= 5) return map[desc - 1];
	if (desc == 900) return "FatalError";
	return NULL;
}

static const char* get_desc_desc_vctl(int desc) {
	const char* const map[] = {
		"A process that requires initialization was called before it was initialized.",
		"The initialization process was called while already initialized.",
		"Unsupported version.",
		"Invalid argument.",
		"Insufficient space.",
	};
	if (desc >= 1 && desc <= 5) return map[desc - 1];
	if (desc == 900) return "A fatal error occurred.";
	return NULL;
}

static const char* get_desc_str_webbrs(int desc) {
	const char* const map[] = {
		"InvalidUrl",
		"UnavailableWebBrowser",
		"LocalCommMode",
	};
	if (desc >= 1 && desc <= 3) return map[desc - 1];
	return NULL;
}

static const char* get_desc_desc_webbrs(int desc) {
	const char* const map[] = {
		"Invalid URL.",
		"Unavailable.",
		"Cannot start during local communication.",	};
	if (desc >= 1 && desc <= 3) return map[desc - 1];
	return NULL;
}

static int application_desc_size = 0;
static const char* const * application_desc_str_map = NULL;
static const char* const * application_desc_desc_map = NULL;

void set_application_desc_map(int size, const char* const * const str_map, const char* const * const desc_map) {
	application_desc_size = size;
	application_desc_str_map = str_map;
	application_desc_desc_map = desc_map;
}

static const char* get_desc_str_application(int desc) {
	if (!application_desc_str_map) return NULL;
	
	if (desc < application_desc_size) return application_desc_str_map[desc];
	
	return NULL;
}

static const char* get_desc_desc_application(int desc) {
	if (!application_desc_desc_map) return NULL;
	
	if (desc < application_desc_size) return application_desc_desc_map[desc];
	
	return NULL;
}

static const char* (*module_desc_str[])(int) = {
	NULL, //"cmn",
	get_desc_str_kern,
	NULL, //"util",
	NULL, //"fs srv",
	NULL, //"ldr srv",
	get_desc_str_tcb,
	get_desc_str_os,
	get_desc_str_dbg,
	get_desc_str_dmnt,
	get_desc_str_pdn,
	NULL, //"gx",
	NULL, //"i2c",
	NULL, //"gpio",
	get_desc_str_dd,
	NULL, //"codec",
	NULL, //"spi",
	NULL, //"pxi",
	get_desc_str_fs,
	NULL, //"di",
	NULL, //"hid",
	get_desc_str_camera,
	NULL, //"pi",
	NULL, //"pm",
	get_desc_str_pmlow,
	NULL, //"fsi",
	get_desc_str_srv,
	get_desc_str_ndm,
	get_desc_str_nwm,
	get_desc_str_socket,
	get_desc_str_ldr,
	NULL, //"acc",
	NULL, //"romfs",
	get_desc_str_am,
	get_desc_str_hio,
	get_desc_str_updater,
	get_desc_str_mic,
	get_desc_str_fnd,
	get_desc_str_mp,
	NULL, //"mpwl",
	get_desc_str_ac,
	get_desc_str_http,
	get_desc_str_dsp,
	get_desc_str_snd,
	get_desc_str_dlp,
	NULL, //"hio low",
	get_desc_str_csnd,
	get_desc_str_ssl,
	NULL, //"am low",
	NULL, //"nex",
	get_desc_str_friends,
	get_desc_str_rdt,
	get_desc_str_applet,
	get_desc_str_nim,
	get_desc_str_ptm,
	get_desc_str_midi,
	NULL, //"mc",
	NULL, //"swc",
	NULL, //"fatfs",
	NULL, //"ngc",
	NULL, //"card",
	NULL, //"card nor",
	NULL, //"sdmc",
	get_desc_str_boss,
	get_desc_str_dbm,
	get_desc_str_cfg,
	NULL, //"ps",
	get_desc_str_cec,
	get_desc_str_ir,
	get_desc_str_uds,
	get_desc_str_pl,
	get_desc_str_cup,
	NULL, //"gyro",
	get_desc_str_mcu,
	get_desc_str_ns,
	get_desc_str_news,
	get_desc_str_ro,
	get_desc_str_gd,
	get_desc_str_cardspi,
	get_desc_str_ec,
	get_desc_str_webbrs,
	NULL, //"test",
	get_desc_str_enc,
	NULL, //"pia",
	NULL, //"act",
	get_desc_str_vctl,
	NULL, //"olv",
	NULL, //"neia",
	NULL, //"npns",

	NULL, //"reserved0",
	NULL, //"reserved1",

	NULL, //"avd",
	get_desc_str_l2b,
	get_desc_str_mvd,
	get_desc_str_nfc,
	NULL, //"uart",
	NULL, //"spm"
	get_desc_str_qtm,
	NULL, //"nfp",
	NULL, //"npt",
};

static const char* (*module_desc_desc[])(int) = {
	NULL, //"cmn",
	get_desc_desc_kern,
	NULL, //"util",
	NULL, //"fs srv",
	NULL, //"ldr srv",
	get_desc_desc_tcb,
	get_desc_desc_os,
	get_desc_desc_dbg,
	get_desc_desc_dmnt,
	get_desc_desc_pdn,
	NULL, //"gx",
	NULL, //"i2c",
	NULL, //"gpio",
	get_desc_desc_dd,
	NULL, //"codec",
	NULL, //"spi",
	NULL, //"pxi",
	get_desc_desc_fs,
	NULL, //"di",
	NULL, //"hid",
	get_desc_desc_camera,
	NULL, //"pi",
	NULL, //"pm",
	get_desc_desc_pmlow,
	NULL, //"fsi",
	get_desc_desc_srv,
	get_desc_desc_ndm,
	get_desc_desc_nwm,
	get_desc_desc_socket,
	get_desc_desc_ldr,
	NULL, //"acc",
	NULL, //"romfs",
	get_desc_desc_am,
	get_desc_desc_hio,
	get_desc_desc_updater,
	get_desc_desc_mic,
	get_desc_desc_fnd,
	get_desc_desc_mp,
	NULL, //"mpwl",
	get_desc_desc_ac,
	get_desc_desc_http,
	get_desc_desc_dsp,
	get_desc_desc_snd,
	get_desc_desc_dlp,
	NULL, //"hio low",
	get_desc_desc_csnd,
	get_desc_desc_ssl,
	NULL, //"am low",
	NULL, //"nex",
	get_desc_desc_friends,
	get_desc_desc_rdt,
	get_desc_desc_applet,
	get_desc_desc_nim,
	get_desc_desc_ptm,
	get_desc_desc_midi,
	NULL, //"mc",
	NULL, //"swc",
	NULL, //"fatfs",
	NULL, //"ngc",
	NULL, //"card",
	NULL, //"card nor",
	NULL, //"sdmc",
	get_desc_desc_boss,
	get_desc_desc_dbm,
	get_desc_desc_cfg,
	NULL, //"ps",
	get_desc_desc_cec,
	get_desc_desc_ir,
	get_desc_desc_uds,
	get_desc_desc_pl,
	get_desc_desc_cup,
	NULL, //"gyro",
	get_desc_desc_mcu,
	get_desc_desc_ns,
	get_desc_desc_news,
	get_desc_desc_ro,
	get_desc_desc_gd,
	get_desc_desc_cardspi,
	get_desc_desc_ec,
	get_desc_desc_webbrs,
	NULL, //"test",
	get_desc_desc_enc,
	NULL, //"pia",
	NULL, //"act",
	get_desc_desc_vctl,
	NULL, //"olv",
	NULL, //"neia",
	NULL, //"npns",

	NULL, //"reserved0",
	NULL, //"reserved1",

	NULL, //"avd",
	get_desc_desc_l2b,
	get_desc_desc_mvd,
	get_desc_desc_nfc,
	NULL, //"uart",
	NULL, //"spm"
	get_desc_desc_qtm,
	NULL, //"nfp",
	NULL, //"npt",
};

const char* get_level_string(int32_t res) {
	int level = CTR_RESULT_GET_LEVEL(res);
	if (CTR_RESULT_IS_FAILURE(res)) {
		if (level < 25 || level > 31) return "Invalid";
		return level_to_string[level - 23];
	}
	if (level <= 1) return level_to_string[level];
	return "Invalid";
}

const char* get_level_description(int32_t res) {
	int level = CTR_RESULT_GET_LEVEL(res);
	if (CTR_RESULT_IS_FAILURE(res)) {
		if (level < 25 || level > 31) return "Invalid Level Value.";
		return level_description[level - 23];
	}
	if (level <= 1) return level_description[level];
	return "Invalid Level Value.";
}

void get_level_formatted(char* dest, size_t size, int32_t res) {
	int level = CTR_RESULT_GET_LEVEL(res);
	snprintf(dest, size, "%s (%d) - %s", get_level_string(res), level, get_level_description(res));
}

const char* get_summary_string(int32_t res) {
	int summary = CTR_RESULT_GET_SUMMARY(res);
	if (summary <= 11) return summary_to_string[summary];
	return "Invalid";
}

const char* get_summary_description(int32_t res) {
	int summary = CTR_RESULT_GET_SUMMARY(res);
	if (summary <= 11) return summary_description[summary];
	return "Invalid Summary Value";
}

void get_summary_formatted(char* dest, size_t size, int32_t res) {
	int summary = CTR_RESULT_GET_SUMMARY(res);
	snprintf(dest, size, "%s (%d) - %s", get_summary_string(res), summary, get_summary_description(res));
}

const char* get_module_string(int32_t res) {
	int module = CTR_RESULT_GET_MODULE(res);
	if (module == 254) return "application";
	if (module <= 98) return module_to_string[module];
	return "invalid";
}

const char* get_module_description(int32_t res) {
	int module = CTR_RESULT_GET_MODULE(res);
	if (module == 254) return "Application";
	if (module <= 98) return module_description[module];
	return "Invalid";
}

void get_module_formatted(char* dest, size_t size, int32_t res) {
	int module = CTR_RESULT_GET_MODULE(res);
	snprintf(dest, size, "%s (%d) - %s", get_module_string(res), module, get_module_description(res));
}

const char* get_description_string(int32_t res) {
	int description = CTR_RESULT_GET_DESCRIPTION(res);
	int module = CTR_RESULT_GET_MODULE(res);
	if (module >= 0 && module <= 98) {
		const char*(*fn)(int) = module_desc_str[module];
		if (fn) {
			const char* s = fn(description);
			if (s) return s;
		}
	}
	if (module == CTR_RESULT_MODULE_APPLICATION) {
		const char* s = get_desc_str_application(description);
		if (s) return s;
	}
	if (description == 0) return "Success";
	if (description >= 1000) return description_to_string[description - 1000];
	return "unknown";
}

const char* get_description_description(int32_t res) {
	int description = CTR_RESULT_GET_DESCRIPTION(res);
	int module = CTR_RESULT_GET_MODULE(res);
	if (module >= 0 && module <= 98) {
		const char*(*fn)(int) = module_desc_desc[module];
		if (fn) {
			const char* s = fn(description);
			if (s) return s;
		}
	}
	if (module == CTR_RESULT_MODULE_APPLICATION) {
		const char* s = get_desc_desc_application(description);
		if (s) return s;
	}
	if (description == 0) return "Succeeded.";
	if (description >= 1000) return description_description[description - 1000];
	return "Unknown.";
}

void get_description_formatted(char* dest, size_t size, int32_t res) {
	int description = CTR_RESULT_GET_DESCRIPTION(res);
	snprintf(dest, size, "%s (%d) - %s", get_description_string(res), description, get_description_description(res));
}
