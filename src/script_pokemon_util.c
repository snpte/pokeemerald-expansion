#include "global.h"
#include "battle.h"
#include "battle_gfx_sfx_util.h"
#include "berry.h"
#include "data.h"
#include "daycare.h"
#include "decompress.h"
#include "event_data.h"
#include "international_string_util.h"
#include "link.h"
#include "link_rfu.h"
#include "main.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "pokedex.h"
#include "pokemon.h"
#include "random.h"
#include "script.h"
#include "sprite.h"
#include "string_util.h"
#include "tv.h"
#include "wild_encounter.h"
#include "constants/items.h"
#include "constants/battle_frontier.h"

static void CB2_ReturnFromChooseHalfParty(void);
static void CB2_ReturnFromChooseBattleFrontierParty(void);
static u16 PickRustboroEgg(void);
static u16 PickWaterStarterEgg(void);
static u16 PickGrassStarterEgg(void);
static u16 PickFireStarterEgg(void);

void HealPlayerParty(void)
{
    u8 i, j;
    u8 ppBonuses;
    u8 arg[4];

    // restore HP.
    for(i = 0; i < gPlayerPartyCount; i++)
    {
        u16 maxHP = GetMonData(&gPlayerParty[i], MON_DATA_MAX_HP);
        arg[0] = maxHP;
        arg[1] = maxHP >> 8;
        SetMonData(&gPlayerParty[i], MON_DATA_HP, arg);
        ppBonuses = GetMonData(&gPlayerParty[i], MON_DATA_PP_BONUSES);

        // restore PP.
        for(j = 0; j < MAX_MON_MOVES; j++)
        {
            arg[0] = CalculatePPWithBonus(GetMonData(&gPlayerParty[i], MON_DATA_MOVE1 + j), ppBonuses, j);
            SetMonData(&gPlayerParty[i], MON_DATA_PP1 + j, arg);
        }

        // since status is u32, the four 0 assignments here are probably for safety to prevent undefined data from reaching SetMonData.
        arg[0] = 0;
        arg[1] = 0;
        arg[2] = 0;
        arg[3] = 0;
        SetMonData(&gPlayerParty[i], MON_DATA_STATUS, arg);
    }
}

u8 ScriptGiveMon(u16 species, u8 level, u16 item, u32 unused1, u32 unused2, u8 unused3)
{
    u16 nationalDexNum;
    int sentToPc;
    u8 heldItem[2];
    struct Pokemon mon;
    u16 targetSpecies;

    if (OW_SYNCHRONIZE_NATURE >= GEN_6 && (gSpeciesInfo[species].eggGroups[0] == EGG_GROUP_NO_EGGS_DISCOVERED || OW_SYNCHRONIZE_NATURE == GEN_7))
        CreateMonWithNature(&mon, species, level, MAX_PER_STAT_IVS, PickWildMonNature());
    else
        CreateMon(&mon, species, level, MAX_PER_STAT_IVS, FALSE, 0, OT_ID_PLAYER_ID, 0);

    heldItem[0] = item;
    heldItem[1] = item >> 8;
    SetMonData(&mon, MON_DATA_HELD_ITEM, heldItem);

    // In case a mon with a form changing item is given. Eg: SPECIES_ARCEUS_NORMAL with ITEM_SPLASH_PLATE will transform into SPECIES_ARCEUS_WATER upon gifted.
    targetSpecies = GetFormChangeTargetSpecies(&mon, FORM_CHANGE_ITEM_HOLD, 0);
    if (targetSpecies != SPECIES_NONE)
    {
        SetMonData(&mon, MON_DATA_SPECIES, &targetSpecies);
        CalculateMonStats(&mon);
    }

    sentToPc = GiveMonToPlayer(&mon);
    nationalDexNum = SpeciesToNationalPokedexNum(species);

    // Don't set Pokédex flag for MON_CANT_GIVE
    switch(sentToPc)
    {
    case MON_GIVEN_TO_PARTY:
    case MON_GIVEN_TO_PC:
        GetSetPokedexFlag(nationalDexNum, FLAG_SET_SEEN);
        GetSetPokedexFlag(nationalDexNum, FLAG_SET_CAUGHT);
        break;
    }
    return sentToPc;
}

u8 ScriptGiveEgg(u16 species)
{
    struct Pokemon mon;
    u8 isEgg;
    bool8 fromHotSprings = TRUE;

    if(species == SPECIES_WOOPER_PALDEAN)
    {
        species = PickRustboroEgg();
        fromHotSprings = FALSE;
    }
    if(species == SPECIES_SQUIRTLE)
    {
        species = PickWaterStarterEgg();
        fromHotSprings = FALSE;
    }
    else if(species == SPECIES_BULBASAUR)
    {
        species = PickGrassStarterEgg();
        fromHotSprings = FALSE;
    }
    else if(species == SPECIES_CHARMANDER)
    {
        species = PickFireStarterEgg();
        fromHotSprings = FALSE;
    }

    CreateEgg(&mon, species, fromHotSprings);
    isEgg = TRUE;
    SetMonData(&mon, MON_DATA_IS_EGG, &isEgg);

    return GiveMonToPlayer(&mon);
}

static u16 PickRustboroEgg(void)
{
    u16 pokes[5];
    u16 rand = Random() % 5;
    pokes[0] = SPECIES_WOOPER_PALDEAN;
    pokes[1] = SPECIES_NYMBLE;
    pokes[2] = SPECIES_MASCHIFF;
    pokes[3] = SPECIES_SHROODLE;
    pokes[4] = SPECIES_WIGLETT;
    return pokes[rand];
}

static u16 PickWaterStarterEgg(void)
{
    u16 pokes[9];
    u16 rand = Random() % 9;
    pokes[0] = SPECIES_SQUIRTLE;
    pokes[1] = SPECIES_TOTODILE;
    pokes[2] = SPECIES_MUDKIP;
    pokes[3] = SPECIES_PIPLUP;
    pokes[4] = SPECIES_OSHAWOTT;
    pokes[5] = SPECIES_FROAKIE;
    pokes[6] = SPECIES_POPPLIO;
    pokes[7] = SPECIES_SOBBLE;
    pokes[8] = SPECIES_QUAXLY;
    return pokes[rand];
}

static u16 PickGrassStarterEgg(void)
{
    u16 pokes[9];
    u16 rand = Random() % 9;
    pokes[0] = SPECIES_BULBASAUR;
    pokes[1] = SPECIES_CHIKORITA;
    pokes[2] = SPECIES_TREECKO;
    pokes[3] = SPECIES_TURTWIG;
    pokes[4] = SPECIES_SNIVY;
    pokes[5] = SPECIES_CHESPIN;
    pokes[6] = SPECIES_ROWLET;
    pokes[7] = SPECIES_GROOKEY;
    pokes[8] = SPECIES_SPRIGATITO;
    return pokes[rand];
}

static u16 PickFireStarterEgg(void)
{
    u16 pokes[9];
    u16 rand = Random() % 9;
    pokes[0] = SPECIES_CHARMANDER;
    pokes[1] = SPECIES_CYNDAQUIL;
    pokes[2] = SPECIES_TORCHIC;
    pokes[3] = SPECIES_CHIMCHAR;
    pokes[4] = SPECIES_TEPIG;
    pokes[5] = SPECIES_FENNEKIN;
    pokes[6] = SPECIES_LITTEN;
    pokes[7] = SPECIES_SCORBUNNY;
    pokes[8] = SPECIES_FUECOCO;
    return pokes[rand];
}

void HasEnoughMonsForDoubleBattle(void)
{
    switch (GetMonsStateToDoubles())
    {
    case PLAYER_HAS_TWO_USABLE_MONS:
        gSpecialVar_Result = PLAYER_HAS_TWO_USABLE_MONS;
        break;
    case PLAYER_HAS_ONE_MON:
        gSpecialVar_Result = PLAYER_HAS_ONE_MON;
        break;
    case PLAYER_HAS_ONE_USABLE_MON:
        gSpecialVar_Result = PLAYER_HAS_ONE_USABLE_MON;
        break;
    }
}

static bool8 CheckPartyMonHasHeldItem(u16 item)
{
    int i;

    for(i = 0; i < PARTY_SIZE; i++)
    {
        u16 species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG);
        if (species != SPECIES_NONE && species != SPECIES_EGG && GetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM) == item)
            return TRUE;
    }
    return FALSE;
}

bool8 DoesPartyHaveEnigmaBerry(void)
{
    bool8 hasItem = CheckPartyMonHasHeldItem(ITEM_ENIGMA_BERRY_E_READER);
    if (hasItem == TRUE)
        GetBerryNameByBerryType(ItemIdToBerryType(ITEM_ENIGMA_BERRY_E_READER), gStringVar1);

    return hasItem;
}

void CreateScriptedWildMon(u16 species, u8 level, u16 item)
{
    u8 heldItem[2];

    ZeroEnemyPartyMons();
    if (OW_SYNCHRONIZE_NATURE > GEN_3)
        CreateMonWithNature(&gEnemyParty[0], species, level, USE_RANDOM_IVS, PickWildMonNature());
    else
        CreateMon(&gEnemyParty[0], species, level, USE_RANDOM_IVS, 0, 0, OT_ID_PLAYER_ID, 0);
    if (item)
    {
        heldItem[0] = item;
        heldItem[1] = item >> 8;
        SetMonData(&gEnemyParty[0], MON_DATA_HELD_ITEM, heldItem);
    }
}
void CreateScriptedDoubleWildMon(u16 species1, u8 level1, u16 item1, u16 species2, u8 level2, u16 item2)
{
    u8 heldItem1[2];
    u8 heldItem2[2];

    ZeroEnemyPartyMons();

    if (OW_SYNCHRONIZE_NATURE > GEN_3)
        CreateMonWithNature(&gEnemyParty[0], species1, level1, 32, PickWildMonNature());
    else
        CreateMon(&gEnemyParty[0], species1, level1, 32, 0, 0, OT_ID_PLAYER_ID, 0);
    if (item1)
    {
        heldItem1[0] = item1;
        heldItem1[1] = item1 >> 8;
        SetMonData(&gEnemyParty[0], MON_DATA_HELD_ITEM, heldItem1);
    }

    if (OW_SYNCHRONIZE_NATURE > GEN_3)
        CreateMonWithNature(&gEnemyParty[1], species2, level2, 32, PickWildMonNature());
    else
        CreateMon(&gEnemyParty[1], species2, level2, 32, 0, 0, OT_ID_PLAYER_ID, 0);
    if (item2)
    {
        heldItem2[0] = item2;
        heldItem2[1] = item2 >> 8;
        SetMonData(&gEnemyParty[1], MON_DATA_HELD_ITEM, heldItem2);
    }
}

void ScriptSetMonMoveSlot(u8 monIndex, u16 move, u8 slot)
{
// Allows monIndex to go out of bounds of gPlayerParty. Doesn't occur in vanilla
#ifdef BUGFIX
    if (monIndex >= PARTY_SIZE)
#else
    if (monIndex > PARTY_SIZE)
#endif
        monIndex = gPlayerPartyCount - 1;

    SetMonMoveSlot(&gPlayerParty[monIndex], move, slot);
}

// Note: When control returns to the event script, gSpecialVar_Result will be
// TRUE if the party selection was successful.
void ChooseHalfPartyForBattle(void)
{
    gMain.savedCallback = CB2_ReturnFromChooseHalfParty;
    VarSet(VAR_FRONTIER_FACILITY, FACILITY_MULTI_OR_EREADER);
    InitChooseHalfPartyForBattle(0);
}

static void CB2_ReturnFromChooseHalfParty(void)
{
    switch (gSelectedOrderFromParty[0])
    {
    case 0:
        gSpecialVar_Result = FALSE;
        break;
    default:
        gSpecialVar_Result = TRUE;
        break;
    }

    SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

void ChoosePartyForBattleFrontier(void)
{
    gMain.savedCallback = CB2_ReturnFromChooseBattleFrontierParty;
    InitChooseHalfPartyForBattle(gSpecialVar_0x8004 + 1);
}

static void CB2_ReturnFromChooseBattleFrontierParty(void)
{
    switch (gSelectedOrderFromParty[0])
    {
    case 0:
        gSpecialVar_Result = FALSE;
        break;
    default:
        gSpecialVar_Result = TRUE;
        break;
    }

    SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

void ReducePlayerPartyToSelectedMons(void)
{
    struct Pokemon party[MAX_FRONTIER_PARTY_SIZE];
    int i;

    CpuFill32(0, party, sizeof party);

    // copy the selected pokemon according to the order.
    for (i = 0; i < MAX_FRONTIER_PARTY_SIZE; i++)
        if (gSelectedOrderFromParty[i]) // as long as the order keeps going (did the player select 1 mon? 2? 3?), do not stop
            party[i] = gPlayerParty[gSelectedOrderFromParty[i] - 1]; // index is 0 based, not literal

    CpuFill32(0, gPlayerParty, sizeof gPlayerParty);

    // overwrite the first 4 with the order copied to.
    for (i = 0; i < MAX_FRONTIER_PARTY_SIZE; i++)
        gPlayerParty[i] = party[i];

    CalculatePlayerPartyCount();
}
