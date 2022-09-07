// Copyright 2010-2021, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "rewriter/environmental_filter_rewriter.h"

#include <string>
#include <vector>

#include "base/system_util.h"
#include "converter/segments.h"
#include "protocol/commands.pb.h"
#include "request/conversion_request.h"
#include "testing/base/public/googletest.h"
#include "testing/base/public/gunit.h"
#include "absl/flags/flag.h"

namespace mozc {
namespace {
constexpr char kKanaSupplement_6_0[] = "\U0001B001";
constexpr char kKanaSupplement_10_0[] = "\U0001B002";
constexpr char kKanaExtendedA_14_0[] = "\U0001B122";

void AddSegment(const std::string &key, const std::string &value,
                Segments *segments) {
  segments->Clear();
  Segment *seg = segments->push_back_segment();
  seg->set_key(key);
  Segment::Candidate *candidate = seg->add_candidate();
  candidate->Init();
  candidate->value = value;
  candidate->content_value = value;
}

void AddSegment(const std::string &key, const std::vector<std::string> &values,
                Segments *segments) {
  Segment *seg = segments->add_segment();
  seg->set_key(key);
  for (const std::string &value : values) {
    Segment::Candidate *candidate = seg->add_candidate();
    candidate->content_key = key;
    candidate->value = value;
    candidate->content_value = value;
  }
}

class EnvironmentalFilterRewriterTest : public ::testing::Test {
 protected:
  EnvironmentalFilterRewriterTest() = default;
  ~EnvironmentalFilterRewriterTest() override = default;

  void SetUp() override {
    SystemUtil::SetUserProfileDirectory(absl::GetFlag(FLAGS_test_tmpdir));
  }
};

TEST_F(EnvironmentalFilterRewriterTest, RemoveTest) {
  EnvironmentalFilterRewriter rewriter;
  Segments segments;
  const ConversionRequest request;

  segments.Clear();
  AddSegment("a", {"a\t1", "a\n2", "a\n\r3"}, &segments);

  EXPECT_TRUE(rewriter.Rewrite(request, &segments));
  EXPECT_EQ(0, segments.conversion_segment(0).candidates_size());
}

TEST_F(EnvironmentalFilterRewriterTest, NoRemoveTest) {
  EnvironmentalFilterRewriter rewriter;
  Segments segments;
  AddSegment("a", {"aa1", "a.a", "a-a"}, &segments);

  const ConversionRequest request;
  EXPECT_FALSE(rewriter.Rewrite(request, &segments));
  EXPECT_EQ(3, segments.conversion_segment(0).candidates_size());
}

TEST_F(EnvironmentalFilterRewriterTest, CandidateFilterTest) {
  {
    EnvironmentalFilterRewriter rewriter;
    commands::Request request;
    ConversionRequest conversion_request;
    conversion_request.set_request(&request);

    Segments segments;
    segments.Clear();
    // All should not be allowed.
    AddSegment("a",
               {kKanaSupplement_6_0, kKanaSupplement_10_0, kKanaExtendedA_14_0},
               &segments);

    EXPECT_TRUE(rewriter.Rewrite(conversion_request, &segments));
    EXPECT_EQ(0, segments.conversion_segment(0).candidates_size());
  }

  {
    EnvironmentalFilterRewriter rewriter;
    commands::Request request;
    request.add_additional_renderable_character_groups(
        commands::Request::EMPTY);
    ConversionRequest conversion_request;
    conversion_request.set_request(&request);

    Segments segments;
    segments.Clear();
    // All should not be allowed.
    AddSegment("a",
               {kKanaSupplement_6_0, kKanaSupplement_10_0, kKanaExtendedA_14_0},
               &segments);

    EXPECT_TRUE(rewriter.Rewrite(conversion_request, &segments));
    EXPECT_EQ(0, segments.conversion_segment(0).candidates_size());
  }

  {
    EnvironmentalFilterRewriter rewriter;
    commands::Request request;
    request.add_additional_renderable_character_groups(
        commands::Request::KANA_SUPPLEMENT_6_0);
    ConversionRequest conversion_request;
    conversion_request.set_request(&request);

    Segments segments;
    segments.Clear();
    // Only first one should be allowed.
    AddSegment("a",
               {kKanaSupplement_6_0, kKanaSupplement_10_0, kKanaExtendedA_14_0},
               &segments);

    EXPECT_TRUE(rewriter.Rewrite(conversion_request, &segments));
    EXPECT_EQ(1, segments.conversion_segment(0).candidates_size());
  }

  {
    EnvironmentalFilterRewriter rewriter;
    commands::Request request;
    request.add_additional_renderable_character_groups(
        commands::Request::KANA_SUPPLEMENT_6_0);
    request.add_additional_renderable_character_groups(
        commands::Request::KANA_SUPPLEMENT_AND_KANA_EXTENDED_A_10_0);
    ConversionRequest conversion_request;
    conversion_request.set_request(&request);

    Segments segments;
    segments.Clear();
    // First and second one should be allowed.
    AddSegment("a",
               {kKanaSupplement_6_0, kKanaSupplement_10_0, kKanaExtendedA_14_0},
               &segments);

    EXPECT_TRUE(rewriter.Rewrite(conversion_request, &segments));
    EXPECT_EQ(2, segments.conversion_segment(0).candidates_size());
  }

  {
    EnvironmentalFilterRewriter rewriter;
    commands::Request request;
    request.add_additional_renderable_character_groups(
        commands::Request::KANA_SUPPLEMENT_6_0);
    request.add_additional_renderable_character_groups(
        commands::Request::KANA_SUPPLEMENT_AND_KANA_EXTENDED_A_10_0);
    request.add_additional_renderable_character_groups(
        commands::Request::KANA_EXTENDED_A_14_0);
    ConversionRequest conversion_request;
    conversion_request.set_request(&request);

    Segments segments;
    segments.Clear();
    // All should be allowed.
    AddSegment("a",
               {kKanaSupplement_6_0, kKanaSupplement_10_0, kKanaExtendedA_14_0},
               &segments);

    EXPECT_FALSE(rewriter.Rewrite(conversion_request, &segments));
    EXPECT_EQ(3, segments.conversion_segment(0).candidates_size());
  }
}

TEST_F(EnvironmentalFilterRewriterTest, NormalizationTest) {
  EnvironmentalFilterRewriter rewriter;
  Segments segments;
  const ConversionRequest request;

  segments.Clear();
  AddSegment("test", "test", &segments);
  EXPECT_FALSE(rewriter.Rewrite(request, &segments));
  EXPECT_EQ("test", segments.segment(0).candidate(0).value);

  segments.Clear();
  AddSegment("きょうと", "京都", &segments);
  EXPECT_FALSE(rewriter.Rewrite(request, &segments));
  EXPECT_EQ("京都", segments.segment(0).candidate(0).value);

  // Wave dash (U+301C) per platform
  segments.Clear();
  AddSegment("なみ", "〜", &segments);
  constexpr char description[] = "[全]波ダッシュ";
  segments.mutable_segment(0)->mutable_candidate(0)->description = description;
#ifdef OS_WIN
  EXPECT_TRUE(rewriter.Rewrite(request, &segments));
  // U+FF5E
  EXPECT_EQ("～", segments.segment(0).candidate(0).value);
  EXPECT_TRUE(segments.segment(0).candidate(0).description.empty());
#else  // OS_WIN
  EXPECT_FALSE(rewriter.Rewrite(request, &segments));
  // U+301C
  EXPECT_EQ("〜", segments.segment(0).candidate(0).value);
  EXPECT_EQ(description, segments.segment(0).candidate(0).description);
#endif  // OS_WIN

  // Wave dash (U+301C) w/ normalization
  segments.Clear();
  AddSegment("なみ", "〜", &segments);
  segments.mutable_segment(0)->mutable_candidate(0)->description = description;

  rewriter.SetNormalizationFlag(TextNormalizer::kAll);
  EXPECT_TRUE(rewriter.Rewrite(request, &segments));
  // U+FF5E
  EXPECT_EQ("～", segments.segment(0).candidate(0).value);
  EXPECT_TRUE(segments.segment(0).candidate(0).description.empty());

  // Wave dash (U+301C) w/o normalization
  segments.Clear();
  AddSegment("なみ", "〜", &segments);
  segments.mutable_segment(0)->mutable_candidate(0)->description = description;

  rewriter.SetNormalizationFlag(TextNormalizer::kNone);
  EXPECT_FALSE(rewriter.Rewrite(request, &segments));
  // U+301C
  EXPECT_EQ("〜", segments.segment(0).candidate(0).value);
  EXPECT_EQ(description, segments.segment(0).candidate(0).description);

  // not normalized.
  segments.Clear();
  // U+301C
  AddSegment("なみ", "〜", &segments);
  segments.mutable_segment(0)->mutable_candidate(0)->attributes |=
      Segment::Candidate::USER_DICTIONARY;
  EXPECT_FALSE(rewriter.Rewrite(request, &segments));
  // U+301C
  EXPECT_EQ("〜", segments.segment(0).candidate(0).value);

  // not normalized.
  segments.Clear();
  // U+301C
  AddSegment("なみ", "〜", &segments);
  segments.mutable_segment(0)->mutable_candidate(0)->attributes |=
      Segment::Candidate::NO_MODIFICATION;
  EXPECT_FALSE(rewriter.Rewrite(request, &segments));
  // U+301C
  EXPECT_EQ("〜", segments.segment(0).candidate(0).value);
}

}  // namespace
}  // namespace mozc
