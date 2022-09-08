//
//  ObjectiveCUtilities.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 11/1/12.
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//

#include "boost/function.hpp"
#include "boost/bind.hpp"

static boost::function<void()> boostFuncFromSelector(SEL selector, id delegate)
{
    typedef void (*func)(id, SEL);
    func impl = (func)[delegate methodForSelector:selector];
    return boost::bind(impl, delegate, selector);
}

template<typename T>
static boost::function<void(T)> boostFuncFromSelector_1(SEL selector, id delegate)
{
    typedef void (*func)(id, SEL, T);
    func impl = (func)[delegate methodForSelector:selector];
    return boost::bind(impl, delegate, selector, _1);
}

template<typename T>
static boost::function<void(T)> boostFuncFromSelector_1(SEL selector, T passedArg, id delegate)
{
    typedef void (*func)(id, SEL, T);
    func impl = (func)[delegate methodForSelector:selector];
    return boost::bind(impl, delegate, selector, passedArg);
}

template<typename T, typename T2>
static boost::function<void(T,T2)> boostFuncFromSelector_2(SEL selector, id delegate)
{
    typedef void (*func)(id, SEL, T, T2);
    func impl = (func)[delegate methodForSelector:selector];
    return boost::bind(impl, delegate, selector, _1, _2);
}

template<typename T, typename T2>
static boost::function<void(T,T2)> boostFuncFromSelector_2(SEL selector, T passedArg1, id delegate)
{
    typedef void (*func)(id, SEL, T, T2);
    func impl = (func)[delegate methodForSelector:selector];
    return boost::bind(impl, delegate, selector, passedArg1, _2);
}

template<typename T, typename T2>
static boost::function<void(T,T2)> boostFuncFromSelector_2(SEL selector,  T passedArg1, T2 passedArg2, id delegate)
{
    typedef void (*func)(id, SEL, T, T2);
    func impl = (func)[delegate methodForSelector:selector];
    return boost::bind(impl, delegate, selector, passedArg1, passedArg2);
}


template<typename T, typename T2, typename T3>
static boost::function<void(T,T2, T3)> boostFuncFromSelector_3(SEL selector, id delegate)
{
    typedef void (*func)(id, SEL, T, T2, T3);
    func impl = (func)[delegate methodForSelector:selector];
    return boost::bind(impl, delegate, selector, _1, _2, _3);
}

template<typename T, typename T2, typename T3, typename T4>
static boost::function<void(T,T2, T3, T4)> boostFuncFromSelector_4(SEL selector, id delegate)
{
    typedef void (*func)(id, SEL, T, T2, T3, T4);
    func impl = (func)[delegate methodForSelector:selector];
    return boost::bind(impl, delegate, selector, _1, _2, _3, _4);
}