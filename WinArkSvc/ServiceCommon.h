#pragma once

enum class MessageType {
	SetAlarm,
	CancelAlarm
};

struct AlarmMessage {
	MessageType Type;
	FILETIME Time;
};