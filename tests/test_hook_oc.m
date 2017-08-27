/**
 *    Copyright 2017 jmpews
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "hookzz.h"
#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#import <mach-o/dyld.h>
#import <dlfcn.h>

@interface HookZz : NSObject

@end

@implementation HookZz

+ (void)load {
  [self zzMethodSwizzlingHook];
}

void objcMethod_pre_call(RegState *rs, ThreadStack *threadstack, CallStack *stack) {
  zpointer t = 0x1234; 
  STACK_SET(stack ,"key_x", t, void *);
  STACK_SET(stack ,"key_y", t, zpointer);
  NSLog(@"hookzz OC-Method: -[ViewController %s]",
        (zpointer)(rs->general.regs.x1));
}

void objcMethod_post_call(RegState *rs, ThreadStack *threadstack, CallStack *stack) {
  zpointer x = STACK_GET(stack, "key_x", void *);
  zpointer y = STACK_GET(stack, "key_y", zpointer);
  NSLog(@"function over, and get 'key_x' is: %p", x);
  NSLog(@"function over, and get 'key_y' is: %p", y);
}

+ (void)zzMethodSwizzlingHook {
  Class hookClass = objc_getClass("UIViewController");
  SEL oriSEL = @selector(viewWillAppear:);
  Method oriMethod = class_getInstanceMethod(hookClass, oriSEL);
  IMP oriImp = method_getImplementation(oriMethod);

  ZzBuildHook((void *)oriImp, NULL, NULL, objcMethod_pre_call, objcMethod_post_call);
  ZzEnableHook((void *)oriImp);

}

@end
