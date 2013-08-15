/*
 * BLELabs BLE112-Protostick Demo Application for Tutorial #3
 * Contact: support@blelabs.com
 * Author: PeterVanPansen
 * Date: 12.08.2013
 * License: Stays as is
 *
 */

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* ++++++++++++++++++++++++++++++++++ BASED ON ++++++++++++++++++++++++++++++ */
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*
 * Bluegigas Bluetooth Smart Demo Application
 * Contact: support@bluegiga.com.
 *
 * This is free software distributed under the terms of the MIT license reproduced below.
 *
 * Copyright (c) 2012, Bluegiga Technologies
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include <cmd_def.h>
#include <config.h>
#include <utils.h>

/* Storage to hold recently received data */
uint8 value_buffer[256] = { 0 };

/* Globals through config.h */
hci_connection_t app_connection;
hci_attclient_t app_attclient;
uint16_t app_state;

/* Serial port stuff */
volatile HANDLE serial_handle;
void output(uint8 len1, uint8* data1, uint16 len2, uint8* data2);
int read_message();

/* For message flow */
int8 wait_for_rsp();
int8 wait_for_evt();

/* Misc */
void solveThisIssue();
void die();

/* Usage */
void print_help() {
	printf("\n\tUsage: BL_T0003\tCOM-port\n");
}

int main(int argc, char *argv[]) {
	/* http://wiki.eclipse.org/CDT/User/FAQ#Eclipse_console_does_not_show_output_on_Windows */
	solveThisIssue();

	/* Give user hint what this program does */
	printf(">> BLELabs BLE112-Protostick Demo Application for Tutorial #3 (www.BLELabs.com) <<\n");
	printf("---> Connect with another device\n");
	printf("---> Read and write characteristics and handles\n");
	printf("---> Activation of a notification service\n");
	printf("---> Catch notifications\n");

	/* Check for serial port name */
	if (argc < 2) {
		print_help();
		exit(-1);
	}

	/* Setting up the Serial Port ... */
	char str[80];
	snprintf(str, sizeof(str) - 1, "\\\\.\\%s", argv[1]);
	serial_handle = CreateFileA(str, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, 0, NULL );

	if (serial_handle == INVALID_HANDLE_VALUE) {
		printf("Error opening serialport %s. %d\n", argv[1], (int) GetLastError());
		exit(-1);
	}

	DCB serialSettings;
	if (GetCommState(serial_handle, &serialSettings)) {
		printf(" \n\tSerial settings:\n\t----------------\n");
		printf("\tBaudRate:%ld\n", serialSettings.BaudRate = CBR_57600);
		printf("\tByteSize:%d\n", serialSettings.ByteSize = 8);
		printf("\tParity:%d\n", serialSettings.Parity = NOPARITY);
		printf("\tStopBits:%d\n", serialSettings.StopBits = ONESTOPBIT);
		printf("\tfOutxCtsFlow:%d\n", serialSettings.fOutxCtsFlow = TRUE);
		printf("\tfRtsControl:%d\n \n", serialSettings.fRtsControl = RTS_CONTROL_ENABLE);
		SetCommState(serial_handle, &serialSettings);
	}
	/* ... finished */

	/* Hook into BGLib */
	bglib_output = output;

	/* Target MAC address */
	bd_addr target = { { 0x00, 0x07, 0x80, 0x6a, 0xf2, 0x8b } };
	/* API needs address reversed */
	reverseArray(target.addr, sizeof(target.addr));

	/* BLE settings */
	app_connection.addr_type = 0;
	app_connection.target = target;
	app_connection.conn_interval_min = 80; /*100ms*/
	app_connection.conn_interval_max = 3200; /*1s*/
	app_connection.timeout = 1000;/*10s*/
	app_connection.latency = 0;/*0ms*/

	/* Provide place to store data */
	app_attclient.value.data = value_buffer;
	app_attclient.value.len = 0;

	/* Initialize status flags */
	app_connection.state = APP_DEVICE_INIT;
	app_attclient.state = 0;
	app_state = 0;

	/* stop previous operation */
	printf("[>] ble_cmd_gap_end_procedure\n");
	ble_cmd_gap_end_procedure();
	if (wait_for_rsp() != APP_OK) {
		die();
	}
	/* No need to wait for an event here */

	/* get connection status,current command will be handled in response */
	printf("[>] ble_cmd_connection_get_status\n");
	ble_cmd_connection_get_status(0);
	if (wait_for_rsp() != APP_OK) {
		die();
	}
	/* No need to wait for an event here */

	/* Connect to target with specific settings */
	printf("[###]Connect to target[###]\n");
	printf("[>] ble_cmd_gap_connect_direct\n");
	ble_cmd_gap_connect_direct(app_connection.target.addr, app_connection.addr_type, app_connection.conn_interval_min,
			app_connection.conn_interval_max, app_connection.timeout, app_connection.latency);
	if (wait_for_rsp() != APP_OK) {
		die();
	}
	wait_for_evt();

	/* Are we connected? */
	if (app_connection.state != APP_DEVICE_CONNECTED) {
		printf("[#] Connecting failed.\n");
		die();
	}

	/* Handle range helpers */
	uint16 handle_start = 0x0001;
	uint16 handle_end = 0xFFFF;

	/* Give me all informations you can get */
	printf("[###]Find Informations[###]\n");
	printf("[>] ble_cmd_attclient_find_information\n");
	ble_cmd_attclient_find_information(app_connection.handle, handle_start, handle_end);
	if (wait_for_rsp() != APP_OK) {
		die();
	}
	wait_for_evt();

	/* Give me all primary services within the whole handle range */
	/* Hint: With handle start and handle end groups can be separated */
	uint8 uuid[] = GATT_PRIMARY_SERVICE_UUID;
	uint8 uuid_len = sizeof(uuid);
	printf("[>] ble_cmd_attclient_read_by_group_type\n");
	ble_cmd_attclient_read_by_group_type(app_connection.handle, handle_start, handle_end, uuid_len, uuid);
	if (wait_for_rsp() != APP_OK) {
		die();
	}
	wait_for_evt();

	/* Now lets get us some values with an UUID i.e. the device name */
	uint8 devicename_uuid[] = GATT_DEVICENAME_UUID;
	uint8 devicename_uuid_len = sizeof(devicename_uuid);

	/* Read device name by its UUID */
	printf("[###]Read target device name by 16bit UUID[###]\n");
	printf("[>] ble_cmd_attclient_read_by_type\n");
	ble_cmd_attclient_read_by_type(app_connection.handle, handle_start, handle_end, devicename_uuid_len,
			devicename_uuid);
	if (wait_for_rsp() != APP_OK) {
		die();
	}
	wait_for_evt();

	/* If we got a value back print it, it must be our targets device name */
	int i = 0;
	if (!issetFlag(app_state, APP_ATTCLIENT_ERROR)) {
		if (issetFlag(app_state, APP_ATTCLIENT_VALUE_PENDING)) {
			clearFlag(app_state, APP_ATTCLIENT_VALUE_PENDING);
			printf("[#] Device name: ");
			for (i = 0; i < app_attclient.value.len; i++) {
				printf("%c", app_attclient.value.data[i]);
			}
			printf("\n");
		}
	} else {
		die();
	}

	/* Write a value with a handle */
	uint8 bgdemo_handle = 20;
	uint8 bgdemo_value[] = {0x12,0x34,0x56};
	uint8 bgdemo_value_len = sizeof(bgdemo_value);

	printf("[###]Write a value by handle[###]\n");
	printf("[>] ble_cmd_attclient_attribute_write\n");
	ble_cmd_attclient_attribute_write(app_connection.handle, bgdemo_handle, bgdemo_value_len, bgdemo_value);
	if (wait_for_rsp() != APP_OK) {
		die();
	}
	wait_for_evt();

	/* Read value with 128-bit UUID */
	uint8 bgdemo_char_uuid[16] = {0};
	uint8 bgdemo_char_uuid_len = sizeof(bgdemo_char_uuid);
	/* String representation of uuid */
	char str_uuid[] = "f1b41cde-dbf5-4acf-8679-ecb8b4dca6fe";
	/* Convert string uuid to uint8 array */
	uuid128StrToArray(str_uuid, bgdemo_char_uuid);
	/* Reverse address to network format */
	reverseArray(bgdemo_char_uuid, bgdemo_char_uuid_len);

	printf("[###]Read a value by 128bit UUID[###]\n");
	printf("[>] ble_cmd_attclient_read_by_type\n");
	ble_cmd_attclient_read_by_type(app_connection.handle, handle_start, handle_end, bgdemo_char_uuid_len,
			bgdemo_char_uuid);
	if (wait_for_rsp() != APP_OK) {
		die();
	}
	wait_for_evt();

	/* If we got a value back print it, it must be our 0xDEADBEEF */
	if (!issetFlag(app_state, APP_ATTCLIENT_ERROR)) {
		if (issetFlag(app_state, APP_ATTCLIENT_VALUE_PENDING)) {
			clearFlag(app_state, APP_ATTCLIENT_VALUE_PENDING);
			printf("[#] Is it 0x123456?: ");
			for (i = 0; i < app_attclient.value.len; i++) {
				printf("%02x", app_attclient.value.data[i]);
			}
			printf("\n");
		}
	} else {
		die();
	}

	/* The data is simply an unsigned 16-bit one in network format, this value will enable the notification */
	uint8 serv_conf_enable[2] = { 0x01, 0x00 };
	uint8 serv_conf_len = sizeof(serv_conf_enable);
	uint8 serv_conf_handle = 17;
	uint8 serv_notification_handle = 16;

	/* Write the data to a handle we already looked up with BLEGUI. It is the handle of the
	 * GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_UUID related to the custom battery service.
	 * This handle is valid for the BGDemo example.
	 */
	printf("[###]Activate Service Notification by handle[###]\n");
	printf("[>] ble_cmd_attclient_attribute_write\n");
	ble_cmd_attclient_attribute_write(app_connection.handle, serv_conf_handle, serv_conf_len, serv_conf_enable);
	if (wait_for_rsp() != APP_OK) {
		die();
	}
	wait_for_evt();

	/* Get notified 10 times ... */
	printf("[###]Watch for notifications and print them if arriving[###]\n");
	int notifications = 0;
	while (notifications < 10) {
		/* We have to check for BGLib messages, this was previously hidden within the wait_for_... functions. */
		if (read_message()) {
			printf("Error reading message\n");
			exit(-1);
		}

		/* Check if everything is fine */
		if (issetFlag(app_state, APP_ATTCLIENT_ERROR)) {
			die();
		};

		/* Check if a notification is pending and if yes check for the handle of the battery service we are looking for */
		if (issetFlag(app_state, APP_ATTCLIENT_NOTIFICATION_PENDING)) {
			clearFlag(app_state, APP_ATTCLIENT_NOTIFICATION_PENDING);

			/* We already know the handle of the custom battery service by looking it up with BLEGUI earlier */
			/* Now lets see if it is the handle we've waited for. */
			if (app_attclient.handle == serv_notification_handle) {
				/* The service characteristics value is simply the battery voltage in 10th of a millivolt.
				 * First put the 16-bit value together and then convert the value to volts. */
				uint16 voltage = (app_attclient.value.data[1] << 8) | app_attclient.value.data[0];
				printf("[#] Notification %d - Battery Voltage: %1.3f Volt\n", notifications, voltage * 0.0001);
				notifications++;
			}
		}
	}

	/* ... then disconnect */
	printf("[###]Disconnect from target[###]\n");
	printf("[>] ble_cmd_connection_disconnect\n");
	ble_cmd_connection_disconnect(app_connection.handle);
	if (wait_for_rsp() != APP_OK) {
		die();
	}
	wait_for_evt();

	/* Loop until the end of time */
	while (1)
		;

	exit(0);
}

int8 wait_for_rsp() {
	setFlag(app_state, APP_COMMAND_PENDING);
	while (issetFlag(app_state, APP_COMMAND_PENDING)) {
		if (read_message()) {
			printf("Error reading message\n");
			return APP_FAILURE;
		}
	}
	return APP_OK;
}

int8 wait_for_evt() {
	setFlag(app_state, APP_ATTCLIENT_PENDING);
	while (issetFlag(app_state, APP_ATTCLIENT_PENDING)) {
		if (read_message()) {
			printf("Error reading message\n");
			return APP_FAILURE;
		}
	}
	return APP_OK;
}

void output(uint8 len1, uint8* data1, uint16 len2, uint8* data2) {
	DWORD written;

	if (!WriteFile(serial_handle, data1, len1, &written, NULL )) {
		printf("ERROR: Writing data. %d\n", (int) GetLastError());
		exit(-1);
	}

	if (!WriteFile(serial_handle, data2, len2, &written, NULL )) {
		printf("ERROR: Writing data. %d\n", (int) GetLastError());
		exit(-1);
	}
}

int read_message() {
	DWORD rread;
	const struct ble_msg *apimsg;
	struct ble_header apihdr;
	unsigned char data[256]; //enough for BLE

	/* read header */
	if (!ReadFile(serial_handle, (unsigned char*) &apihdr, 4, &rread, NULL )) {
		return GetLastError();
	}
	if (!rread)
		return 0;

	/* read rest if needed */
	if (apihdr.lolen) {
		if (!ReadFile(serial_handle, data, apihdr.lolen, &rread, NULL )) {
			return GetLastError();
		}
	}
	apimsg = ble_get_msg_hdr(apihdr);
	if (!apimsg) {
		printf("ERROR: Message not found:%d:%d\n", (int) apihdr.cls, (int) apihdr.command);
		return -1;
	}
	apimsg->handler(data);

	return 0;
}

inline void die() {
	printf("Failure. End of program...\n");
	exit(-1);
}

/* Eclipse Auto Formatter fails after setvbuf line, that's why these statements are at the end of the file */
inline void solveThisIssue() {
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
}
