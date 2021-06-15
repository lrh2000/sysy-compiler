#include <vector>
#include <stack>
#include "asm.h"

struct AsmLabelInfo
{
  AsmLabelInfo(size_t nr_lines, size_t nr_labels)
    : jump_dest(), label_inst(), pending_labels()
  {
    jump_dest.resize(nr_lines, ~0ul);
    label_inst.resize(nr_labels, ~0ul);
  }

  void reset(void)
  {
    std::fill(jump_dest.begin(), jump_dest.end(), ~0ul);
    std::fill(label_inst.begin(), label_inst.end(), ~0ul);
  }

  std::vector<size_t> jump_dest;
  std::vector<size_t> label_inst;
  std::stack<size_t> pending_labels;
};

void AsmLine::fill_label_info(size_t pos, AsmLabelInfo *info) const
{
  while (!info->pending_labels.empty())
  {
    info->label_inst[info->pending_labels.top()] = pos;
    info->pending_labels.pop();
  }
}

void AsmLocalLabel::fill_label_info(size_t pos, AsmLabelInfo *info) const
{
  info->pending_labels.push(labelid.id);
}

void AsmJumpInst::fill_label_info(size_t pos, AsmLabelInfo *info) const
{
  AsmLine::fill_label_info(pos, info);

  info->jump_dest[pos] = target.id;
}

bool AsmLine::is_label(void) const
{
  return false;
}

bool AsmLocalLabel::is_label(void) const
{
  return true;
}

void AsmLine::mark_label_used(std::vector<bool> &used_labels) const
{ /* nothing */ }

void AsmJumpInst::mark_label_used(std::vector<bool> &used_labels) const
{
  used_labels[target.id] = true;
}

void AsmBranchInst::mark_label_used(std::vector<bool> &used_labels) const
{
  used_labels[target.id] = true;
}

std::unique_ptr<AsmLine> AsmLine::clone_if_jump(void) const
{
  return nullptr;
}

std::unique_ptr<AsmLine> AsmJumpInst::clone_if_jump(void) const
{
  return std::make_unique<AsmJumpInst>(target);
}

std::unique_ptr<AsmLine> AsmJumpRegInst::clone_if_jump(void) const
{
  return std::make_unique<AsmJumpRegInst>(rs);
}

AsmLine *AsmLine::update_label(
    const std::vector<size_t> &rules,
    const std::vector<bool> &used)
{
  return this;
}

AsmLine *AsmLocalLabel::update_label(
    const std::vector<size_t> &rules,
    const std::vector<bool> &used)
{
  if (!used[labelid.id])
    return nullptr;
  return new AsmLocalLabel(AsmLabelId(rules[labelid.id]));
}

AsmLine *AsmJumpInst::update_label(
    const std::vector<size_t> &rules,
    const std::vector<bool> &used)
{
  return new AsmJumpInst(AsmLabelId(rules[target.id]));
}

AsmLine *AsmBranchInst::update_label(
    const std::vector<size_t> &rules,
    const std::vector<bool> &used)
{
  return new AsmBranchInst(op, rs1, rs2, AsmLabelId(rules[target.id]));
}

void AsmFile::relabel(void)
{
  AsmLabelInfo info(lines.size(), num_labels);
  for (size_t i = 0; i < lines.size(); ++i)
    lines[i]->fill_label_info(i, &info);

  for (size_t i = lines.size() - 1; ~i; --i)
  {
    if (~info.jump_dest[i] == 0)
      continue;
    size_t j = info.label_inst[info.jump_dest[i]];
    assert(~j != 0);

    if (j == i)
      continue;
    if (auto newline = lines[j]->clone_if_jump();
        newline != nullptr)
      lines[i] = std::move(newline);
  }

  info.reset();
  for(size_t i = 0; i < lines.size(); ++i)
    lines[i]->fill_label_info(i, &info);

  for (size_t i = lines.size() - 1, j = lines.size();
      ~i;
      --i)
  {
    if (!lines[i]->is_label())
      j = i;
    if (~info.jump_dest[i] == 0)
      continue;
    if (info.label_inst[info.jump_dest[i]] == j)
      lines[i] = nullptr;
  }

  lines.erase(std::remove_if(lines.begin(), lines.end(),
        [] (const std::unique_ptr<AsmLine> &line) { return !line; }),
      lines.end());

  std::vector<std::pair<size_t, size_t>> labels;
  for (size_t i = 0; i < info.label_inst.size(); ++i)
    labels.emplace_back(info.label_inst[i], i);
  std::sort(labels.begin(), labels.end());

  std::vector<bool> used;
  used.resize(num_labels);
  for (size_t i = 0; i < lines.size(); ++i)
    lines[i]->mark_label_used(used);

  std::vector<size_t> rules;
  rules.resize(num_labels, ~0ul);

  size_t last_line = ~0ul;
  size_t last_label = ~0ul;
  for (size_t i = 0; i < labels.size(); ++i)
  {
    if (!used[labels[i].second])
      continue;
    if (last_line != labels[i].first) {
      last_line = labels[i].first;
      ++last_label;
    }
    rules[labels[i].second] = last_label;
  }

  std::vector<std::unique_ptr<AsmLine>> newlines;
  for (auto &line : lines)
  {
    AsmLine *newline = line->update_label(rules, used);
    if (newline == line.get())
      newlines.emplace_back(std::move(line));
    else if (newline != nullptr)
      newlines.emplace_back(std::unique_ptr<AsmLine>(newline));
  }

  lines = std::move(newlines);
}
