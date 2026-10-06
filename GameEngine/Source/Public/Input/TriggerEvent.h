#pragma once

enum class ETriggerEvent
{
	None,
	Started,      //이번 프레임에 발동 시작
	Ongoing,      //안 씀
	Triggered,    //매 프레임에 눌림
	Completed,    //이번 프레임에 발동 끝남
	Canceled,     //안 씀
};