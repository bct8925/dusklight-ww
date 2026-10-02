// PC addition (not in the decomp). JKR_NEW/JKR_DELETE and the JKR operator new/delete overloads,
// split out of JKRHeap.h so global.h can make them visible to all game code.
#ifndef JKRNEW_H
#define JKRNEW_H

#include <cstddef>
#if TARGET_PC
#include <new>
#include <type_traits>
#include <utility>
#endif

class JKRHeap;

// On PC, game allocations go through JKR_NEW/JKR_DELETE (operator new/delete overloads taking a
// JKRHeapToken), so the global operator new/delete stay the C runtime's for the shell, aurora and
// the standard library. On the GameCube these macros are plain new/delete. Adapted from dusklight.
#if TARGET_PC
enum class JKRHeapToken {
    Dummy
};

inline void* operator new(size_t, JKRHeapToken, void* where) noexcept {
    return where;
}

inline void* operator new[](size_t, JKRHeapToken, void* where) noexcept {
    return where;
}

#define JKR_NEW new (JKRHeapToken::Dummy)
#define JKR_NEW_ARRAY(type, count) jkrNewArray(count, std::in_place_type<type>)
#define JKR_NEW_ARGS(...) new (JKRHeapToken::Dummy, __VA_ARGS__)
#define JKR_NEW_ARRAY_ARGS(type, count, ...) jkrNewArray(count, std::in_place_type<type>, __VA_ARGS__)
#define JKR_DELETE(expr) jkrDelete(expr)
#define JKR_DELETE_ARRAY(expr) jkrDeleteArray(expr)
#define JKR_HEAP_TOKEN , JKRHeapToken::Dummy
#define JKR_HEAP_TOKEN_PARAM , JKRHeapToken
#else
#define JKR_NEW new
#define JKR_NEW_ARRAY(type, count) new type[count]
#define JKR_NEW_ARGS(...) new (__VA_ARGS__ )
#define JKR_NEW_ARRAY_ARGS(type, count, ...) new (__VA_ARGS__ ) type[count]
#define JKR_DELETE(expr) delete (expr)
#define JKR_DELETE_ARRAY(expr) delete[] (expr)
#define JKR_HEAP_TOKEN
#define JKR_HEAP_TOKEN_PARAM
#endif

void* operator new(size_t size JKR_HEAP_TOKEN_PARAM) IF_DUSK(noexcept);
void* operator new(size_t size JKR_HEAP_TOKEN_PARAM, int alignment) IF_DUSK(noexcept);
void* operator new(size_t size JKR_HEAP_TOKEN_PARAM, JKRHeap* heap, int alignment) IF_DUSK(noexcept);

// On PC, these new[] overloads are only used to catch usages of JKR_NEW with [].
void* operator new[](size_t size JKR_HEAP_TOKEN_PARAM) IF_DUSK(noexcept);
void* operator new[](size_t size JKR_HEAP_TOKEN_PARAM, int alignment) IF_DUSK(noexcept);
void* operator new[](size_t size JKR_HEAP_TOKEN_PARAM, JKRHeap* heap, int alignment) IF_DUSK(noexcept);

void operator delete(void* ptr JKR_HEAP_TOKEN_PARAM) IF_DUSK(noexcept);
void operator delete[](void* ptr JKR_HEAP_TOKEN_PARAM) IF_DUSK(noexcept);

#if TARGET_PC
template<typename T>
void jkrDelete(T* ptr) IF_DUSK(noexcept) {
    if (ptr == nullptr) {
        return;
    }
    ptr->~T();

    if constexpr (requires { T::operator delete(ptr, JKRHeapToken::Dummy); }) {
        T::operator delete(ptr, JKRHeapToken::Dummy);
    } else if constexpr (requires { T::operator delete(ptr, sizeof(T), JKRHeapToken::Dummy); }) {
        T::operator delete(ptr, sizeof(T), JKRHeapToken::Dummy);
    } else if constexpr (requires { T::operator delete(ptr); }) {
        T::operator delete(ptr);
    } else if constexpr (requires { T::operator delete(ptr, sizeof(T)); }) {
        T::operator delete(ptr, sizeof(T));
    } else {
        operator delete(ptr, JKRHeapToken::Dummy);
    }
}

template<>
inline void jkrDelete(void* ptr) IF_DUSK(noexcept) {
    if (ptr == nullptr) {
        return;
    }

    operator delete(ptr, JKRHeapToken::Dummy);
}

template<typename... Args>
bool constexpr newArgsHasCustomAlignment() {
    return false;
}

template<int>
constexpr bool newArgsHasCustomAlignment() {
    return true;
}

template<JKRHeap*, int>
constexpr bool newArgsHasCustomAlignment() {
    return true;
}

template<typename T, typename... Args>
T* jkrNewArray(size_t count, std::in_place_type_t<T>, Args&&... args) IF_DUSK(noexcept) {
    size_t allocSize = count * sizeof(T);
    if constexpr (!std::is_trivially_destructible<T>()) {
        static_assert(
            !newArgsHasCustomAlignment<Args...>(),
            "jkrNewArray cannot currently handle non-trivially-destructible array allocations with custom alignment");

        allocSize += sizeof(size_t);
    }

    void* ptr = operator new(allocSize, JKRHeapToken::Dummy, args...);
    if (!ptr) {
        return nullptr;
    }

    T* dataPtr;
    if constexpr (!std::is_trivially_destructible<T>()) {
        auto length = static_cast<size_t*>(ptr);
        *length = count;
        dataPtr = reinterpret_cast<T*>(length + 1);
    } else {
        dataPtr = static_cast<T*>(ptr);
    }

    if constexpr (!std::is_trivially_constructible<T>()) {
        for (int i = 0; i < count; ++i) {
            new (dataPtr + i) T();
        }
    }

    return dataPtr;
}

template<typename T>
void jkrDeleteArray(T* pointer) IF_DUSK(noexcept) {
    if (pointer == nullptr) {
        return;
    }

    if constexpr (!std::is_trivially_destructible<T>()) {
        auto countPtr = reinterpret_cast<size_t*>(pointer) - 1;
        auto count = *countPtr;

        for (int i = 0; i < count; ++i) {
            (pointer + i)->~T();
        }

        operator delete(countPtr, JKRHeapToken::Dummy);
    } else {
        operator delete(pointer, JKRHeapToken::Dummy);
    }
}

template<>
inline void jkrDeleteArray(void* pointer) IF_DUSK(noexcept) {
    if (pointer == nullptr) {
        return;
    }

    operator delete(pointer, JKRHeapToken::Dummy);
}
#endif

#endif /* JKRNEW_H */
