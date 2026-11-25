#include "Config.h"
#include "Player.h"
#include "PlayerScript.h"
#include "WorldScript.h"
#include "WorldSession.h"

#include <algorithm>
#include <array>
#include <initializer_list>

namespace
{
enum class RidingTier : uint8
{
    ApprenticeRiding,
    JourneymanRiding,
    ExpertRiding,
    ArtisanRiding,
    ColdWeatherFlying,
    Count
};

struct SpellRule
{
    uint8 playerClass;
    int8 team;
    uint32 spell;
    uint8 level;
    std::initializer_list<uint32> requirements;
    std::initializer_list<uint32> rewards;
    bool talentRank;
    bool questReward;
};

struct ProficiencyRule
{
    uint8 playerClass;
    uint32 spell;
    uint8 level;
};

struct RidingRule
{
    int8 team;
    int8 race;
    int8 playerClass;
    uint32 spell;
    uint8 level;
    uint32 requirement;
    bool questReward;
    RidingTier tier;
};

struct TotemRule
{
    uint32 item;
    uint8 level;
};

#include "AutoAprendizajeData.inc"

struct Settings
{
    bool enabled = false;
    bool gamemasters = true;
    bool classSpells = true;
    bool questSpells = true;
    bool talentRanks = true;
    bool proficiencies = false;
    bool totems = true;
    std::array<bool, static_cast<std::size_t>(RidingTier::Count)> riding{};
};

Settings settings;

void LoadSettings()
{
    settings.enabled = sConfigMgr->GetOption<bool>("AutoAprendizaje.Enable", false);
    settings.gamemasters = sConfigMgr->GetOption<bool>("AutoAprendizaje.EnableGamemaster", true);
    settings.classSpells = sConfigMgr->GetOption<bool>("AutoAprendizaje.EnableClassSpells", true);
    settings.questSpells = sConfigMgr->GetOption<bool>("AutoAprendizaje.EnableSpellsFromQuests", true);
    settings.talentRanks = sConfigMgr->GetOption<bool>("AutoAprendizaje.EnableTalentRanks", true);
    settings.proficiencies = sConfigMgr->GetOption<bool>("AutoAprendizaje.EnableProficiencies", false);
    settings.totems = sConfigMgr->GetOption<bool>("AutoAprendizaje.EnableShamanTotems", true);
    settings.riding = {
        sConfigMgr->GetOption<bool>("AutoAprendizaje.EnableApprenticeRiding", false),
        sConfigMgr->GetOption<bool>("AutoAprendizaje.EnableJourneymanRiding", false),
        sConfigMgr->GetOption<bool>("AutoAprendizaje.EnableExpertRiding", false),
        sConfigMgr->GetOption<bool>("AutoAprendizaje.EnableArtisanRiding", false),
        sConfigMgr->GetOption<bool>("AutoAprendizaje.EnableColdWeatherFlying", false)
    };
}

bool CanTeach(Player const* player)
{
    return settings.enabled && (settings.gamemasters || player->GetSession()->GetSecurity() == SEC_PLAYER);
}

bool KnowsAll(Player const* player, std::initializer_list<uint32> spells)
{
    return std::all_of(spells.begin(), spells.end(), [player](uint32 spell) { return player->HasSpell(spell); });
}

void TeachClassSpells(Player* player)
{
    if (!settings.classSpells)
        return;

    for (SpellRule const& rule : ClassSpells)
    {
        if (rule.playerClass != player->getClass() || (rule.team != -1 && rule.team != player->GetTeamId())
            || player->GetLevel() < rule.level || (rule.questReward && !settings.questSpells)
            || (rule.talentRank && !settings.talentRanks) || !KnowsAll(player, rule.requirements))
            continue;

        if (rule.rewards.size())
        {
            if (!KnowsAll(player, rule.rewards))
                player->CastSpell(player, rule.spell, true);
        }
        else if (!player->HasSpell(rule.spell))
            player->learnSpell(rule.spell);
    }
}

void TeachProficiencies(Player* player)
{
    if (!settings.proficiencies)
        return;

    for (ProficiencyRule const& rule : Proficiencies)
        if (rule.playerClass == player->getClass() && player->GetLevel() >= rule.level
            && !player->HasSpell(rule.spell))
            player->learnSpell(rule.spell);
}

void TeachRiding(Player* player)
{
    for (RidingRule const& rule : Riding)
    {
        if (!settings.riding[static_cast<std::size_t>(rule.tier)]
            || (rule.team != -1 && rule.team != player->GetTeamId())
            || (rule.race != -1 && rule.race != player->getRace())
            || (rule.playerClass != -1 && rule.playerClass != player->getClass())
            || player->GetLevel() < rule.level || (rule.questReward && !settings.questSpells)
            || (rule.requirement && !player->HasSpell(rule.requirement)) || player->HasSpell(rule.spell))
            continue;

        player->learnSpell(rule.spell);
    }
}

void GrantTotems(Player* player)
{
    if (!settings.classSpells || !settings.questSpells || !settings.totems || player->getClass() != CLASS_SHAMAN)
        return;

    for (TotemRule const& rule : Totems)
        if (player->GetLevel() >= rule.level && !player->HasItemCount(rule.item, 1, true))
            player->AddItem(rule.item, 1);
}

void ApplyLearning(Player* player)
{
    if (!CanTeach(player))
        return;

    TeachClassSpells(player);
    GrantTotems(player);
    TeachProficiencies(player);
    TeachRiding(player);
}

class AutoAprendizajeWorldScript : public WorldScript
{
public:
    AutoAprendizajeWorldScript() : WorldScript("AutoAprendizajeWorldScript", {WORLDHOOK_ON_AFTER_CONFIG_LOAD})
    {
        LoadSettings();
    }

    void OnAfterConfigLoad(bool) override
    {
        LoadSettings();
    }
};

class AutoAprendizajePlayerScript : public PlayerScript
{
public:
    AutoAprendizajePlayerScript() : PlayerScript("AutoAprendizajePlayerScript",
        {PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LEVEL_CHANGED, PLAYERHOOK_ON_PLAYER_LEARN_TALENTS})
    {
    }

    void OnPlayerLogin(Player* player) override
    {
        ApplyLearning(player);
    }

    void OnPlayerLevelChanged(Player* player, uint8) override
    {
        ApplyLearning(player);
    }

    void OnPlayerLearnTalents(Player* player, uint32, uint32, uint32) override
    {
        if (CanTeach(player))
            TeachClassSpells(player);
    }
};
}

void Addmod_auto_aprendizajeScripts()
{
    new AutoAprendizajeWorldScript();
    new AutoAprendizajePlayerScript();
}
