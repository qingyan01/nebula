/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "exec/mutate/InsertVerticesExecutor.h"
#include "planner/Mutate.h"
#include "service/ExecutionContext.h"
#include "common/clients/storage/GraphStorageClient.h"


namespace nebula {
namespace graph {

folly::Future<Status> InsertVerticesExecutor::execute() {
    return insertVertices().ensure([this]() { UNUSED(this); });
}

folly::Future<Status> InsertVerticesExecutor::insertVertices() {
    dumpLog();

    auto *ivNode = asNode<InsertVertices>(node());
    return ectx()->getStorageClient()->addVertices(ivNode->space(),
            ivNode->getVertices(), ivNode->getPropNames(), ivNode->getOverwritable())
        .via(runner())
        .then([this](storage::StorageRpcResponse<storage::cpp2::ExecResponse> resp) {
            auto completeness = resp.completeness();
            if (completeness != 100) {
                const auto& failedCodes = resp.failedParts();
                for (auto it = failedCodes.begin(); it != failedCodes.end(); it++) {
                    LOG(ERROR) << "Insert vertices failed, error "
                               << storage::cpp2::_ErrorCode_VALUES_TO_NAMES.at(it->second)
                               << ", part " << it->first;
                }
                return Status::Error("Insert vertices not complete, completeness: %d",
                                      completeness);
            }
            finish(Value());
            return Status::OK();
        });
}
}   // namespace graph
}   // namespace nebula