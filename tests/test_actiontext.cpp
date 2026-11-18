// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#include "ActionText.hpp"

#include <gtest/gtest.h>

using Varjo::actionDescription;

TEST(ActionTextTest, templateShouldBeRemoved) { EXPECT_EQ(L"Copying new files", actionDescription(L"Copying new files File: [1], Directory: [9], Size: [6]")); }

TEST(ActionTextTest, labelOfSeveralWordsShouldBeRemoved)
{
    EXPECT_EQ(L"Searching for related applications", actionDescription(L"Searching for related applications Found application: [1]"));
}

TEST(ActionTextTest, descriptionWithoutTemplateShouldStay) { EXPECT_EQ(L"Evaluating launch conditions", actionDescription(L"Evaluating launch conditions")); }

TEST(ActionTextTest, templateAloneShouldGiveNoDescription)
{
    EXPECT_EQ(L"", actionDescription(L"Feature: [1]"));
    EXPECT_EQ(L"", actionDescription(L""));
}
