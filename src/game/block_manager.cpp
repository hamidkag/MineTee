/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <game/block_manager.h>
#include <engine/external/json-parser/json.h>
#include <cstdio>

bool CBlockManager::Init(char *pData, int DataSize)
{
	char aError[256];

	json_settings JsonSettings;
	mem_zero(&JsonSettings, sizeof(JsonSettings));
	json_value *pJsonData = json_parse_ex(&JsonSettings, pData, DataSize, aError);
	if (!pJsonData)
	{
		dbg_msg("BlockManager", "Error: %s", aError);
		return false;
	}

	for (std::size_t k=0; k<pJsonData->u.array.length&&k<256; k++)
	{
		m_aBlocks[k].Reset();
		CBlockInfo *pBlockInfo = &m_aBlocks[k];

		json_value *pJsonObject = pJsonData->u.array.values[k];
		if (!pJsonObject)
			return false;

		str_copy(pBlockInfo->m_aName, (*pJsonObject)["name"], sizeof(pBlockInfo->m_aName));
		pBlockInfo->m_Health = ((*pJsonObject)["health"].type == json_none)?1:(*pJsonObject)["health"].u.integer;
		pBlockInfo->m_LightSize = ((*pJsonObject)["lightSize"].type == json_none)?0:(*pJsonObject)["lightSize"].u.integer;
		pBlockInfo->m_CraftNum = ((*pJsonObject)["craftNum"].type == json_none)?1:(*pJsonObject)["craftNum"].u.integer;
		pBlockInfo->m_Gravity = ((*pJsonObject)["gravity"].type == json_none)?false:(*pJsonObject)["gravity"].u.boolean;
		pBlockInfo->m_PlayerCollide = ((*pJsonObject)["playerCollide"].type == json_none)?true:(*pJsonObject)["playerCollide"].u.boolean;
		pBlockInfo->m_RandomActions = ((*pJsonObject)["randomActions"].type == json_none)?true:(*pJsonObject)["randomActions"].u.integer;
		pBlockInfo->m_HalfTile = ((*pJsonObject)["halfTile"].type == json_none)?false:(*pJsonObject)["halfTile"].u.boolean;
		pBlockInfo->m_Damage = ((*pJsonObject)["damage"].type == json_none)?false:(*pJsonObject)["damage"].u.boolean;
		pBlockInfo->m_OnPut = ((*pJsonObject)["onPut"].type == json_none)?k:(*pJsonObject)["onPut"].u.integer;
		pBlockInfo->m_OnWear = ((*pJsonObject)["onWear"].type == json_none)?-1:(*pJsonObject)["onWear"].u.integer;
		pBlockInfo->m_OnSun = ((*pJsonObject)["onSun"].type == json_none)?-1:(*pJsonObject)["onSun"].u.integer;

		int items = (*pJsonObject)["lightColor"].u.array.length;
		if (items == 3)
		{
			pBlockInfo->m_LightColor.r = (float)((*pJsonObject)["lightColor"].u.array.values[0]->u.dbl)/255.0f;
			pBlockInfo->m_LightColor.g = (float)((*pJsonObject)["lightColor"].u.array.values[1]->u.dbl)/255.0f;
			pBlockInfo->m_LightColor.b = (float)((*pJsonObject)["lightColor"].u.array.values[2]->u.dbl)/255.0f;
		}

		items = (*pJsonObject)["place"].u.array.length;
		if (items == 8)
		{
			const json_value &JsonArrayPlace = (*pJsonObject)["place"];
			array<int> IndexList;
			for (int i=0; i<items; i++)
			{
				IndexList.clear();
				json_value &JsonArray = *JsonArrayPlace.u.array.values[i];
				for (std::size_t e=0; e<JsonArray.u.array.length; IndexList.add(JsonArray.u.array.values[e++]->u.integer));
				pBlockInfo->m_vPlace.add(IndexList);
			}
		}

		items = (*pJsonObject)["mutations"].u.array.length;
		if (items%2 == 0)
		{
			const json_value &JsonArrayPlace = (*pJsonObject)["mutations"];
			array<int> IndexList;
			for (int i=0; i<items; i++)
			{
				IndexList.clear();
				json_value &JsonArray = *JsonArrayPlace.u.array.values[i];
				for (std::size_t e=0; e<JsonArray.u.array.length; IndexList.add(JsonArray.u.array.values[e++]->u.integer));
				pBlockInfo->m_vMutations.add(IndexList);
			}
		}

		items = (*pJsonObject)["craft"].u.object.length;
		for (int i=0; i<items; i++)
		{
			int CraftBlockID = -1;
			int CraftBlockAmount = (*pJsonObject)["craft"].u.object.values[i].value->u.integer;
			sscanf((const char *)(*pJsonObject)["craft"].u.object.values[i].name, "%d", &CraftBlockID);

			pBlockInfo->m_vCraft.insert(std::pair<int, unsigned char>(CraftBlockID, CraftBlockAmount));
		}

		items = (*pJsonObject)["onCook"].u.object.length;
		for (int i=0; i<items; i++)
		{
			int CookBlockID = -1;
			int CookBlockAmount = (*pJsonObject)["onCook"].u.object.values[i].value->u.integer;
			sscanf((const char *)(*pJsonObject)["onCook"].u.object.values[i].name, "%d", &CookBlockID);

			pBlockInfo->m_vOnCook.insert(std::pair<int, unsigned char>(CookBlockID, CookBlockAmount));
		}

		items = (*pJsonObject)["onBreak"].u.object.length;
		for (int i=0; i<items; i++)
		{
			int BreakBlockID = -1;
			int BreakBlockAmount = (*pJsonObject)["onBreak"].u.object.values[i].value->u.integer;
			sscanf((const char *)(*pJsonObject)["onBreak"].u.object.values[i].name, "%d", &BreakBlockID);

			pBlockInfo->m_vOnBreak.insert(std::pair<int, unsigned char>(BreakBlockID, BreakBlockAmount));
		}

		if (pBlockInfo->m_LightSize < 0)
			pBlockInfo->m_LightSize = 0;
		if (pBlockInfo->m_CraftNum < 0)
			pBlockInfo->m_CraftNum = 0;
	}

	json_value_free(pJsonData);

    return true;
}

CBlockManager::CBlockInfo* CBlockManager::GetBlockInfo(unsigned char BlockID)
{
	if (BlockID < 0 || BlockID >= 256)
		return 0x0;

	return &m_aBlocks[BlockID];
}

bool CBlockManager::IsFluid(int BlockID, int *pType)
{
	int ftype = 0;
	if (BlockID >= CBlockManager::WATER_A && BlockID <= CBlockManager::WATER_D)
		ftype = FLUID_WATER;
	else if (BlockID >= CBlockManager::LAVA_A && BlockID <= CBlockManager::LAVA_D)
		ftype = FLUID_LAVA;

	if (pType)
		*pType = ftype;

	return (ftype != 0);
}
