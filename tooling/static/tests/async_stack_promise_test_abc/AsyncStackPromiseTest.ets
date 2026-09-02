/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

native function observeAsyncStack(value: int): int;

let awaitContinuation: Promise<void> | undefined = undefined;
let awaitResolve: ((value: PromiseLike<undefined> | undefined) => void) | undefined = undefined;

function alreadySettled(): int {
    waitForCompletion((): Promise<void> => Promise.resolve().then<void>((): void => {
        observeAsyncStack(1);
    }));
    return 0;
}

function submitAwaitFirst(): int {
    let promise = new Promise<void>((resolve: (value: PromiseLike<undefined> | undefined) => void,
            reject: (error: Error) => void): void => {
        awaitResolve = resolve;
    });
    awaitContinuation = (async (): Promise<void> => {
        await promise;
        observeAsyncStack(3);
    })();
    return 0;
}

function resolveAwaitFirst(): int {
    let resolve = awaitResolve;
    if (resolve != undefined) {
        resolve(undefined);
    }
    awaitResolve = undefined;

    let continuation = awaitContinuation;
    if (continuation != undefined) {
        waitForCompletion((): Promise<void> => continuation);
    }
    awaitContinuation = undefined;
    return 0;
}

function alreadyRejectedCatch(): int {
    waitForCompletion((): Promise<void> => Promise.reject().catch<void>((): void => {
        observeAsyncStack(4);
    }));
    return 0;
}

function alreadySettledFinally(): int {
    waitForCompletion((): Promise<void> => Promise.resolve().finally((): void => {
        observeAsyncStack(5);
    }));
    return 0;
}
