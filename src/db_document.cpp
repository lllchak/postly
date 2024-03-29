#include "db_document.h"
#include "utils.h"

TDBDocument TDBDocument::FromProto(const postly::TDocumentProto& proto) {
    TDBDocument document;

    document.Filename = proto.filename();
    document.Url = proto.url();
    document.Host = GetHostFromUrl(proto.url());
    document.SiteName = proto.sitename();
    document.PublicationTime = proto.pub_time();
    document.FetchTime = proto.fetch_time();
    document.TTL = proto.ttl();
    document.Title = proto.title();
    document.Text = proto.text();
    document.Description = proto.desc();
    document.Language = proto.language();
    document.Category = proto.category();
    document.IsNasty = proto.is_nasty();

    for (const auto& link : proto.out_links()) {
        document.OutLinks.push_back(link);
    }

    for (const auto& embedding : proto.embeddings()) {
        const auto& value_proto = embedding.value();
        TEmbedding value(value_proto.cbegin(), value_proto.cend());

        const auto [_, ok] =
            document.Embeddings.try_emplace(embedding.key(), std::move(value));
        ENSURE(ok, "Duplicated key");
    }

    return document;
}

bool TDBDocument::FromProtoString(const std::string& value, TDBDocument* document) {
    postly::TDocumentProto proto;
    if (proto.ParseFromString(value)) {
        *document = FromProto(proto);
        return true;
    }
    return false;
}

bool TDBDocument::ParseFromArray(const void* data, const int size, TDBDocument* document) {
    postly::TDocumentProto proto;
    if (proto.ParseFromArray(data, size)) {
        *document = FromProto(proto);
        return true;
    }
    return false;
}

postly::TDocumentProto TDBDocument::ToProto() const {
    postly::TDocumentProto proto;

    proto.set_filename(Filename);
    proto.set_url(Url);
    proto.set_sitename(SiteName);
    proto.set_pub_time(PublicationTime);
    proto.set_fetch_time(FetchTime);
    proto.set_ttl(TTL);
    proto.set_title(Title);
    proto.set_text(Text);
    proto.set_desc(Description);
    proto.set_language(Language);
    proto.set_category(Category);
    proto.set_is_nasty(IsNasty);

    for (const auto& [key, val] : Embeddings) {
        auto* embeddingProto = proto.add_embeddings();
        embeddingProto->set_key(key);
        *embeddingProto->mutable_value() = {val.cbegin(), val.cend()};
    }
    for (const auto& link : OutLinks) {
        proto.add_out_links(link);
    }

    return proto;
}

nlohmann::json TDBDocument::ToJson() const {
    nlohmann::json json({
        {"url", Url},
        {"site_name", SiteName},
        {"timestamp", FetchTime},
        {"title", Title},
        {"description", Description},
        {"file_name", GetFilename(Filename)},
        {"text", Text},
        {"language", Language},
        {"category", Category}
    });

    if (!OutLinks.empty()) {
        json["out_links"] = OutLinks;
    }

    return json;
}

bool TDBDocument::ToProtoString(std::string* protoString) const {
    return ToProto().SerializeToString(protoString);
}
