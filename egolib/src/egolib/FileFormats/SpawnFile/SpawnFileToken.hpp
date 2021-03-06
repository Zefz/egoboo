#pragma once

#include "egolib/FileFormats/SpawnFile/SpawnFileTokenKind.hpp"

class SpawnFileToken : public id::token<SpawnFileTokenKind>
{
public:
    SpawnFileToken
        (
            SpawnFileTokenKind kind,
            const id::location& startLocation,
            const std::string& lexeme = std::string()
        );
};
