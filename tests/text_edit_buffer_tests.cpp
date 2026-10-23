#include "check.h"

#include <metin2/ui/text_edit_buffer.h>

#include <string>

using namespace metin2;

namespace {
    auto buffer(const char32_t *text) -> text_edit_buffer {
        text_edit_buffer b;
        b.set_text(text);
        return b;
    }

    auto select(text_edit_buffer &b, std::size_t begin, std::size_t end) -> void {
        b.set_caret(begin, false);
        b.set_caret(end, true);
    }
}

TEST(edit_buffer_inserts_at_caret) {
    text_edit_buffer b = buffer(U"ab");
    b.set_caret(1, false);
    CHECK(b.insert_char(U'x'));
    CHECK(b.text() == U"axb");
    CHECK_EQ(b.caret(), std::size_t{2});
    CHECK_EQ(b.utf8(), std::string("axb"));
}

TEST(edit_buffer_typing_replaces_selection) {
    text_edit_buffer b = buffer(U"hello");
    select(b, 1, 4);
    CHECK(b.has_selection());
    CHECK(b.insert_char(U'X'));
    CHECK(b.text() == U"hXo");
    CHECK(!b.has_selection());
    CHECK_EQ(b.caret(), std::size_t{2});
}

TEST(edit_buffer_backspace_and_delete_without_selection) {
    text_edit_buffer b = buffer(U"abc");
    b.set_caret(2, false);
    CHECK(b.erase_backward());
    CHECK(b.text() == U"ac");
    CHECK_EQ(b.caret(), std::size_t{1});
    CHECK(b.erase_forward());
    CHECK(b.text() == U"a");
    CHECK(!b.erase_forward());
    b.set_caret(0, false);
    CHECK(!b.erase_backward());
}

TEST(edit_buffer_backspace_and_delete_erase_the_selection) {
    text_edit_buffer b = buffer(U"abcdef");
    select(b, 1, 4);
    CHECK(b.erase_backward());
    CHECK(b.text() == U"aef");
    CHECK_EQ(b.caret(), std::size_t{1});
    select(b, 2, 1);
    CHECK(b.erase_forward());
    CHECK(b.text() == U"af");
    CHECK_EQ(b.caret(), std::size_t{1});
}

TEST(edit_buffer_word_jumps) {
    text_edit_buffer b = buffer(U"foo  bar baz");
    b.move_left(false, true);
    CHECK_EQ(b.caret(), std::size_t{9});
    b.move_left(false, true);
    CHECK_EQ(b.caret(), std::size_t{5});
    b.move_left(false, true);
    CHECK_EQ(b.caret(), std::size_t{0});
    b.move_right(false, true);
    CHECK_EQ(b.caret(), std::size_t{5});
    b.move_right(true, true);
    CHECK_EQ(b.selection().first, std::size_t{5});
    CHECK_EQ(b.selection().second, std::size_t{9});
}

TEST(edit_buffer_home_and_end_extend_with_shift) {
    text_edit_buffer b = buffer(U"abc");
    b.move_home(true);
    CHECK_EQ(b.selection().first, std::size_t{0});
    CHECK_EQ(b.selection().second, std::size_t{3});
    b.move_end(false);
    CHECK(!b.has_selection());
    CHECK_EQ(b.caret(), std::size_t{3});
    b.move_left(false, false);
    b.move_left(true, false);
    CHECK_EQ(b.selection().first, std::size_t{1});
    b.move_right(false, false); // collapses to the selection end
    CHECK(!b.has_selection());
    CHECK_EQ(b.caret(), std::size_t{2});
}

TEST(edit_buffer_byte_limit_refuses_a_whole_character) {
    text_edit_buffer b;
    b.set_max_bytes(3);
    CHECK(b.insert_char(U'a'));
    CHECK(b.insert_char(U'ä'));  // two bytes
    CHECK(!b.insert_char(U'€')); // three bytes do not fit
    CHECK(!b.insert_char(U'b'));
    CHECK_EQ(b.utf8().size(), std::size_t{3});
}

TEST(edit_buffer_visible_limit_ignores_tags) {
    text_edit_buffer b;
    b.set_max_visible_bytes(3);
    CHECK(b.insert_text(U"|cffff0000ab|r"));
    CHECK(b.insert_char(U'c'));
    CHECK(!b.insert_char(U'd'));
    CHECK_EQ(b.visible_length(), std::size_t{3});

    text_edit_buffer raw;
    raw.set_max_bytes(3);
    CHECK(!raw.insert_text(U"|cffff0000ab|r"));
}

TEST(edit_buffer_number_mode_filters_typing_and_paste) {
    text_edit_buffer b;
    b.set_number_mode(true);
    CHECK(!b.insert_char(U'a'));
    CHECK(b.insert_char(U'7'));
    CHECK(b.insert_text(U"1a2"));
    CHECK(b.text() == U"712");
}

TEST(edit_buffer_select_all_copy_and_secret) {
    text_edit_buffer b = buffer(U"abc");
    b.select_all();
    CHECK(b.copy() == U"abc");
    b.set_secret(true);
    CHECK(b.visible_text() == U"***");
    b.select_all();
    CHECK(b.copy().empty());
    CHECK(b.cut().empty());
    CHECK(b.text() == U"abc");
    b.set_secret(false);
    b.select_all();
    CHECK(b.cut() == U"abc");
    CHECK(b.text().empty());
}

TEST(edit_buffer_caret_clamps_and_set_text_clears_the_selection) {
    text_edit_buffer b = buffer(U"abc");
    b.set_caret(99, false);
    CHECK_EQ(b.caret(), std::size_t{3});
    select(b, 0, 2);
    b.set_text(U"xy");
    CHECK(!b.has_selection());
    CHECK_EQ(b.caret(), std::size_t{2});
}

TEST(edit_buffer_steps_over_tags_whole) {
    text_edit_buffer b = buffer(U"a|Hitem:1:0|h[Sword]|hb");
    CHECK_EQ(b.visible_length(), std::size_t{9});
    CHECK(b.visible_text() == U"a[Sword]b");
    b.move_left(false, false);
    CHECK_EQ(b.caret(), std::size_t{8});
    b.move_left(false, false);
    CHECK_EQ(b.caret(), std::size_t{1});
    b.set_caret(4, false); // inside the link: snaps to its nearest end
    CHECK_EQ(b.caret(), std::size_t{1});
    b.set_caret(7, false);
    CHECK_EQ(b.caret(), std::size_t{8});
    CHECK(b.erase_backward());
    CHECK(b.text() == U"ab");

    text_edit_buffer bars = buffer(U"x||y");
    CHECK_EQ(b.visible_length(), std::size_t{2});
    CHECK_EQ(bars.visible_length(), std::size_t{3});
    bars.set_caret(2, false);
    CHECK(bars.erase_backward());
    CHECK(bars.text() == U"xy");

    text_edit_buffer colored = buffer(U"|cff00ff00hi|r!");
    CHECK(colored.visible_text() == U"hi!");
    colored.set_caret(2, false);
    CHECK(colored.erase_backward());
    CHECK(colored.text() == U"!");
}

TEST(edit_buffer_doubles_a_typed_bar_outside_secret_mode) {
    text_edit_buffer b;
    CHECK(b.insert_char(U'|'));
    CHECK(b.text() == U"||");
    CHECK_EQ(b.visible_length(), std::size_t{1});
    text_edit_buffer secret;
    secret.set_secret(true);
    CHECK(secret.insert_char(U'|'));
    CHECK(secret.text() == U"|");
}

TEST(edit_buffer_insert_text_keeps_whole_atoms_under_the_limit) {
    text_edit_buffer b;
    b.set_max_bytes(10);
    CHECK(b.insert_text(U"ab|Hi:1|h[S]|h"));
    CHECK(b.text() == U"ab");
    CHECK(!b.insert_text(U"|Hi:1|h[S]|h"));
    CHECK(b.insert_text(U"cd\nef")); // control characters are dropped
    CHECK(b.text() == U"abcdef");
}

TEST(edit_buffer_select_word_at) {
    text_edit_buffer b = buffer(U"foo bar");
    b.select_word_at(5);
    CHECK_EQ(b.selection().first, std::size_t{4});
    CHECK_EQ(b.selection().second, std::size_t{7});
    b.select_word_at(3);
    CHECK_EQ(b.selection().first, std::size_t{3});
    CHECK_EQ(b.selection().second, std::size_t{4});
    text_edit_buffer link = buffer(U"a |Hi:1|h[Long Sword]|h b");
    link.select_word_at(4);
    CHECK_EQ(link.selection().first, std::size_t{2});
    CHECK_EQ(link.selection().second, std::size_t{14});
}
