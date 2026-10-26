/*
Copyright 2006 - 2022 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#if defined(_LINKVM)

typedef enum KVM_MouseCursors
{
	KVM_MouseCursor_NONE		= 14,
	KVM_MouseCursor_NOTALLOWED	= 15,
	KVM_MouseCursor_COL_RESIZE	= 16,
	KVM_MouseCursor_NOCHANGE	= -1,
	KVM_MouseCursor_ARROW		= 0, 
	KVM_MouseCursor_APPSTARTING = 1,
	KVM_MouseCursor_CROSS 		= 2,
	KVM_MouseCursor_HAND		= 3,
	KVM_MouseCursor_HELP		= 4,
	KVM_MouseCursor_IBEAM 		= 5,
	KVM_MouseCursor_NO 			= 6,
	KVM_MouseCursor_SIZEALL 	= 7,
	KVM_MouseCursor_SIZENESW 	= 8,
	KVM_MouseCursor_SIZENS		= 9,
	KVM_MouseCursor_SIZENWSE	= 10,
	KVM_MouseCursor_SIZEWE 		= 11,
	KVM_MouseCursor_UPARROW		= 12,
	KVM_MouseCursor_WAIT		= 13
}KVM_MouseCursors;

void KVM_InitMouseCursors(void* pendingPackets);
void KVM_UnInitMouseCursors();
// Priorité à l'utilisateur du poste (cahier des charges prise en main § 9.2).
void Priority_SetMode(int mode);	// 0 = utilisateur (defaut), 1 = technicien
int  Priority_GetMode(void);
void Priority_Reset(void);			// vide les appuis tenus (fin de session / changement de bureau)
// Pointeur du technicien montré chez l'utilisateur (§ 9.2 B) : position relayée à l'overlay (bandeau) par mémoire
// partagée, jamais injectée. visible : 1 = montrer, 0 = cacher. Cleanup ferme le canal (fin de session KVM).
void ShowPointer_Set(int x, int y, int visible);
void ShowPointer_Cleanup(void);
void MouseAction(double absX, double absY, int button, short wheel);
void KeyAction(unsigned char keycode, int up);
void KeyActionUnicode(WORD unicode, int up);
int TouchInit();
void TouchUnInit();
int TouchAction1(unsigned char id, unsigned int flags, unsigned short x, unsigned short y);
int TouchAction2(char* data, int datalen, int scaling);

#endif
