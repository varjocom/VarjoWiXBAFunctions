// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#include "ErrorPrompt.hpp"

#include <gtest/gtest.h>

using namespace Varjo;

namespace
{

void expectAnswers(const PromptButtons& buttons, std::initializer_list<int> answers);

}  // namespace

TEST(ErrorPromptTest, buttonsShouldFollowTheMessageBoxType)
{
    expectAnswers(promptButtons(MB_OK), {IDOK});
    expectAnswers(promptButtons(MB_OKCANCEL), {IDOK, IDCANCEL});
    expectAnswers(promptButtons(MB_ABORTRETRYIGNORE), {IDABORT, IDRETRY, IDIGNORE});
    expectAnswers(promptButtons(MB_YESNOCANCEL), {IDYES, IDNO, IDCANCEL});
    expectAnswers(promptButtons(MB_YESNO), {IDYES, IDNO});
    expectAnswers(promptButtons(MB_RETRYCANCEL), {IDRETRY, IDCANCEL});
    expectAnswers(promptButtons(MB_CANCELTRYCONTINUE), {IDCANCEL, IDTRYAGAIN, IDCONTINUE});
}

TEST(ErrorPromptTest, iconAndOtherFlagsShouldNotChangeTheButtons)
{
    // The disk space error of Windows Installer comes with Retry and Cancel and no icon.
    expectAnswers(promptButtons(MB_RETRYCANCEL | MB_ICONERROR | MB_SETFOREGROUND), {IDRETRY, IDCANCEL});
}

TEST(ErrorPromptTest, unknownTypeShouldShowOk) { expectAnswers(promptButtons(0x7), {IDOK}); }

TEST(ErrorPromptTest, defaultButtonShouldComeFromTheHint)
{
    EXPECT_EQ(0u, promptButtons(MB_ABORTRETRYIGNORE).defaultIndex);
    EXPECT_EQ(1u, promptButtons(MB_ABORTRETRYIGNORE | MB_DEFBUTTON2).defaultIndex);
    EXPECT_EQ(2u, promptButtons(MB_ABORTRETRYIGNORE | MB_DEFBUTTON3).defaultIndex);
}

TEST(ErrorPromptTest, defaultButtonBeyondTheLastShouldBeTheFirst)
{
    EXPECT_EQ(0u, promptButtons(MB_RETRYCANCEL | MB_DEFBUTTON3).defaultIndex);
    EXPECT_EQ(0u, promptButtons(MB_OK | MB_DEFBUTTON4).defaultIndex);
}

TEST(ErrorPromptTest, closingShouldAnswerWithTheButtonThatStops)
{
    EXPECT_EQ(IDCANCEL, closeAnswer(promptButtons(MB_RETRYCANCEL)));
    EXPECT_EQ(IDCANCEL, closeAnswer(promptButtons(MB_YESNOCANCEL)));
    EXPECT_EQ(IDCANCEL, closeAnswer(promptButtons(MB_CANCELTRYCONTINUE)));
    EXPECT_EQ(IDABORT, closeAnswer(promptButtons(MB_ABORTRETRYIGNORE)));
    EXPECT_EQ(IDNO, closeAnswer(promptButtons(MB_YESNO)));
    EXPECT_EQ(IDOK, closeAnswer(promptButtons(MB_OK)));
}

TEST(ErrorPromptTest, buttonsShouldBeAlignedToTheRight)
{
    const PromptButtons one = promptButtons(MB_OK);
    EXPECT_EQ(2u, promptButtonSlot(one, 0));
    const PromptButtons two = promptButtons(MB_RETRYCANCEL);
    EXPECT_EQ(1u, promptButtonSlot(two, 0));
    EXPECT_EQ(2u, promptButtonSlot(two, 1));
    const PromptButtons three = promptButtons(MB_YESNOCANCEL);
    EXPECT_EQ(0u, promptButtonSlot(three, 0));
    EXPECT_EQ(2u, promptButtonSlot(three, 2));
}

TEST(ErrorPromptTest, onlyRetryIgnoreTryAgainAndContinueShouldGoOn)
{
    for (const int answer : {IDRETRY, IDIGNORE, IDTRYAGAIN, IDCONTINUE}) {
        EXPECT_TRUE(continuesPastMessage(answer));
    }
    for (const int answer : {IDOK, IDCANCEL, IDABORT, IDYES, IDNO}) {
        EXPECT_FALSE(continuesPastMessage(answer));
    }
}

namespace
{

void expectAnswers(const PromptButtons& buttons, std::initializer_list<int> answers)
{
    ASSERT_EQ(answers.size(), buttons.count);
    size_t i = 0;
    for (const int answer : answers) {
        EXPECT_EQ(answer, buttons.answers[i++]);
    }
}

}  // namespace
