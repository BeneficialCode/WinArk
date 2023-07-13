# Coding Conventions
Having any coding conventions is good for consistency and clarity, but the actual conventions vary, of course. The following coding conventions are used in this project.

* Windows API functions are used with a double colon prefix. Example: `::CreateFile`.
* Type names use Pascal casing(first letter is capital, and every word starts with a capital letter as well.) Examples: Book,SolidBrush. The exception is UI related classes that start with a capital 'C'; this is for consistency with WTL.
* Private member variables in C++ classes start with an underscore and use camel casing (first letter is small and subsequent words start with capital letter). Examples:`_size,_isRunning`. The exception is for WTL classes where private member variable names start with m_. This is for consistency with ATL/WTL style.
* Variable names do not use the old Hungarian notation. However, there may be some occasional exceptions, such as an h prefix for a handle and a p prefix for a pointer.
* Functions names follow the Windows API convention to use Pascal casing.
* When common data types are needed such as vectors, the C++ standard library is used unless there is good reason to use something else.
* The project uses the Windows Implementation Library(WIL) from Microsoft, released in a Nuget package. Theis library contains helpful types for easier working with the Windows API.
* The project uses the Windows Template Library(WTL) to simplify UI-related code. If you need more details on native Windows UI development, consult the classic "Programming Windows",5th edition,by Charles Petzold.

Hunarian notation uses prefixes to make variable names hint at their type. Examples: `szName,dwValue`. This convention is now considered obsolete, although parameter names and structure members in Windows API use it a lot.

# C++ Usage
The code in this project make some use of C++. Here are the main C++ features we used:

* The `nullptr` keyword, representing a true `NULL` pointer.
* The `auto` keyword allows type inference when declaring and initializing variables. This is useful to reduce clutter, save some typing, and focus on the important parts of the code.
* The `new` and `delete` operators
* Socped enums(enum class).
* Classes with member variables and functions.
* Templates, where they make sense.
* Constructors and destructors, especially for building RAII (Resource Acquisition Is Initialization) types.

Accoding to Pavel Yosifovich(2020,pp.30-31) <<Windows 10 System Programming Part 1>>.