// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <memory>
#include <mutex>

namespace Common {

template <typename T>
class Singleton {
public:
    static T& Instance() {
        static T instance;
        return instance;
    }

protected:
    Singleton() = default;
    ~Singleton() = default;

private:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;
};

template <typename T>
class SingletonCore {
public:
    static T& Instance() {
        std::call_once(initFlag_, &SingletonCore::Init);
        return *instance_;
    }

protected:
    SingletonCore() = default;
    virtual ~SingletonCore() = default;

private:
    static void Init() {
        instance_ = std::unique_ptr<T>(new T());
    }

    static std::unique_ptr<T> instance_;
    static std::once_flag initFlag_;
};

template <typename T>
std::unique_ptr<T> SingletonCore<T>::instance_ = nullptr;

template <typename T>
std::once_flag SingletonCore<T>::initFlag_;

} // namespace Common