// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

BE_NAMESPACE_BEGIN

// indexes are based on Vec4Color::table

// color escape macro
#define C_COLOR_INDEX(c)            ((c) - '0')

// color escape macro
#define WC_COLOR_INDEX(c)           ((c) - L'0')

// color escape macro
#define UC_COLOR_INDEX(c)           ((c) - U'0')

// color escape character
const int C_COLOR_ESCAPE            = '^';
const int C_COLOR_DEFAULT           = '0';
const int C_COLOR_RED               = '1';
const int C_COLOR_GREEN             = '2';
const int C_COLOR_YELLOW            = '3';
const int C_COLOR_BLUE              = '4';
const int C_COLOR_CYAN              = '5';
const int C_COLOR_MAGENTA           = '6';
const int C_COLOR_WHITE             = '7';
const int C_COLOR_GRAY              = '8';
const int C_COLOR_BLACK             = '9';
const int C_COLOR_ORANGE            = ':';
const int C_COLOR_PINK              = ';';
const int C_COLOR_LAWN              = '<';
const int C_COLOR_MINT              = '=';
const int C_COLOR_VIOLET            = '>';
const int C_COLOR_TEAL              = '?';

// color escape character in wide character
const int WC_COLOR_ESCAPE           = L'^';
const int WC_COLOR_DEFAULT          = L'0';
const int WC_COLOR_RED              = L'1';
const int WC_COLOR_GREEN            = L'2';
const int WC_COLOR_YELLOW           = L'3';
const int WC_COLOR_BLUE             = L'4';
const int WC_COLOR_CYAN             = L'5';
const int WC_COLOR_MAGENTA          = L'6';
const int WC_COLOR_WHITE            = L'7';
const int WC_COLOR_GRAY             = L'8';
const int WC_COLOR_BLACK            = L'9';
const int WC_COLOR_ORANGE           = L':';
const int WC_COLOR_PINK             = L';';
const int WC_COLOR_LAWN             = L'<';
const int WC_COLOR_MINT             = L'=';
const int WC_COLOR_VIOLET           = L'>';
const int WC_COLOR_TEAL             = L'?';

// color escape character in unicode
const uint32_t UC_COLOR_ESCAPE      = U'^';
const uint32_t UC_COLOR_DEFAULT     = U'0';
const uint32_t UC_COLOR_RED         = U'1';
const uint32_t UC_COLOR_GREEN       = U'2';
const uint32_t UC_COLOR_YELLOW      = U'3';
const uint32_t UC_COLOR_BLUE        = U'4';
const uint32_t UC_COLOR_CYAN        = U'5';
const uint32_t UC_COLOR_MAGENTA     = U'6';
const uint32_t UC_COLOR_WHITE       = U'7';
const uint32_t UC_COLOR_GRAY        = U'8';
const uint32_t UC_COLOR_BLACK       = U'9';
const uint32_t UC_COLOR_ORANGE      = U':';
const uint32_t UC_COLOR_PINK        = U';';
const uint32_t UC_COLOR_LAWN        = U'<';
const uint32_t UC_COLOR_MINT        = U'=';
const uint32_t UC_COLOR_VIOLET      = U'>';
const uint32_t UC_COLOR_TEAL        = U'?';

// color escape string
#define S_COLOR_DEFAULT             "^0"
#define S_COLOR_RED                 "^1"
#define S_COLOR_GREEN               "^2"
#define S_COLOR_YELLOW              "^3"
#define S_COLOR_BLUE                "^4"
#define S_COLOR_CYAN                "^5"
#define S_COLOR_MAGENTA             "^6"
#define S_COLOR_WHITE               "^7"
#define S_COLOR_GRAY                "^8"
#define S_COLOR_BLACK               "^9"
#define S_COLOR_ORANGE              "^:"
#define S_COLOR_PINK                "^;"
#define S_COLOR_LAWN                "^<"
#define S_COLOR_MINT                "^="
#define S_COLOR_VIOLET              "^>"
#define S_COLOR_TEAL                "^?"

// color escape string 
#define WS_COLOR_DEFAULT            L"^0"
#define WS_COLOR_RED                L"^1"
#define WS_COLOR_GREEN              L"^2"
#define WS_COLOR_YELLOW             L"^3"
#define WS_COLOR_BLUE               L"^4"
#define WS_COLOR_CYAN               L"^5"
#define WS_COLOR_MAGENTA            L"^6"
#define WS_COLOR_WHITE              L"^7"
#define WS_COLOR_GRAY               L"^8"
#define WS_COLOR_BLACK              L"^9"
#define WS_COLOR_ORANGE             L"^:"
#define WS_COLOR_PINK               L"^;"
#define WS_COLOR_LAWN               L"^<"
#define WS_COLOR_MINT               L"^="
#define WS_COLOR_VIOLET             L"^>"
#define WS_COLOR_TEAL               L"^?"

BE_NAMESPACE_END
