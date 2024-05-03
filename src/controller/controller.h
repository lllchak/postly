#pragma once

#include "../annotator/annotator.h"
#include "../clustering/clusterer.h"
#include "../atomic/atomic.h"
#include "../ranker/ranker.h"

#include <memory>

#include <drogon/HttpController.h>
#include <rocksdb/db.h>

class TController : public drogon::HttpController<TController, false> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(TController::Threads, "/threads", drogon::Get);
        ADD_METHOD_TO(TController::Put, "/{fname}", drogon::Put);
        ADD_METHOD_TO(TController::Delete, "/{fname}", drogon::Delete);
        ADD_METHOD_TO(TController::Post, "/{fname}", drogon::Post);
        ADD_METHOD_TO(TController::Get, "/{fname}", drogon::Get);
    METHOD_LIST_END

    void Init(
        const TAtomic<TIndex>* index,
        rocksdb::DB* db,
        std::unique_ptr<TAnnotator> annotator,
        std::unique_ptr<TRanker> ranker);

    void Put(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& fname) const;
    void Delete(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& fname) const;
    void Threads(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;
    void Get(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& fname) const;
    void Post(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& fname) const;

private:
    bool IsReady(
        std::function<void(const drogon::HttpResponsePtr&)> &&callback) const;
    std::optional<TDBDocument> GetDBDocFromReq(
        const drogon::HttpRequestPtr& req,
        const std::string& fname) const;
    bool IndexDBDoc(
        const TDBDocument& doc,
        const std::string& fname) const;
    drogon::HttpStatusCode GetCode(
        const std::string& fname,
        drogon::HttpStatusCode createdCode,
        drogon::HttpStatusCode existedCode) const;

private:
    std::atomic<bool> Initialized{false};

    const TAtomic<TIndex>* Index;

    rocksdb::DB* DB;
    std::unique_ptr<TAnnotator> Annotator;
    std::unique_ptr<TRanker> Ranker;
};
