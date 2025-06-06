#include "duckdb/parser/parsed_data/create_type_info.hpp"
#include "duckdb/catalog/catalog_entry/schema_catalog_entry.hpp"
#include "duckdb/catalog/catalog_entry/table_catalog_entry.hpp"
#include "duckdb/catalog/catalog.hpp"
#include "duckdb/common/extra_type_info.hpp"

namespace duckdb {

CreateTypeInfo::CreateTypeInfo() : CreateInfo(CatalogType::TYPE_ENTRY), bind_function(nullptr) {
}
CreateTypeInfo::CreateTypeInfo(string name_p, LogicalType type_p, bind_logical_type_function_t bind_function_p)
    : CreateInfo(CatalogType::TYPE_ENTRY), name(std::move(name_p)), type(std::move(type_p)),
      bind_function(bind_function_p) {
}

unique_ptr<CreateInfo> CreateTypeInfo::Copy() const {
	auto result = make_uniq<CreateTypeInfo>();
	CopyProperties(*result);
	result->name = name;
	result->type = type;
	if (query) {
		result->query = query->Copy();
	}
	result->bind_function = bind_function;
	return std::move(result);
}

string CreateTypeInfo::ToString() const {
	string result = "";
	result += "CREATE";
	if (on_conflict == OnCreateConflict::REPLACE_ON_CONFLICT) {
		result += " OR REPLACE ";
	}
	if (temporary) {
		result += " TEMPORARY";
	}
	result += " TYPE ";
	if (on_conflict == OnCreateConflict::IGNORE_ON_CONFLICT) {
		result += " IF NOT EXISTS ";
	}
	result += QualifierToString(temporary ? "" : catalog, schema, name);
	if (type.id() == LogicalTypeId::ENUM) {
		auto &values_insert_order = EnumType::GetValuesInsertOrder(type);
		idx_t size = EnumType::GetSize(type);

		result += " AS ENUM ( ";
		for (idx_t i = 0; i < size; i++) {
			result += "'" + values_insert_order.GetValue(i).ToString() + "'";
			if (i != size - 1) {
				result += ", ";
			}
		}
		result += " );";
	} else if (type.id() == LogicalTypeId::INVALID) {
		// CREATE TYPE mood AS ENUM (SELECT 'happy')
		D_ASSERT(query);
		result += " AS ENUM (" + query->ToString() + ")";
	} else if (type.id() == LogicalTypeId::USER) {
		result += " AS ";
		auto extra_info = type.AuxInfo();
		D_ASSERT(extra_info);
		D_ASSERT(extra_info->type == ExtraTypeInfoType::USER_TYPE_INFO);
		auto &user_info = extra_info->Cast<UserTypeInfo>();
		result += QualifierToString(user_info.catalog, user_info.schema, user_info.user_type_name);
	} else {
		result += " AS ";
		result += type.ToString();
	}
	result += ";";
	return result;
}

} // namespace duckdb
