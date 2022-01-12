#include "ncnnloader.h"
#include "convert.h"
#include "fmt1.h"
#include <functional>

void ncnnLoader::fileToNodes(const std::string& filename, std::deque<Node>& nodes)
{
    nodes.clear();
    auto str = convert::readStringFromFile(filename);
    str = convert::replaceAllSubString(str, "\r", "");
    auto lines = convert::splitString(str, "\n");
    int layer_count = 0, blob_count = 0;
    if (lines.size() >= 2)
    {
        auto v = convert::findNumbers<int>(lines[1]);
        layer_count = v[0];
        blob_count = v[1];
    }

    for (int i = 2; i < lines.size(); i++)
    {
        auto v = convert::splitString(lines[i], " ");
        if (v.size() >= 2)
        {
            Node node;
            node.title = v[1];
            node.type = v[0];

            int input_count = atoi(v[2].c_str());
            int output_count = atoi(v[3].c_str());

            for (int i_in = 0; i_in < input_count; i_in++)
            {
                node.in.push_back(v[4 + i_in]);
            }
            for (int i_out = 0; i_out < output_count; i_out++)
            {
                node.out.push_back(v[4 + input_count + i_out]);
            }
            for (int i_rest = 4 + input_count + output_count; i_rest < v.size(); i_rest++)
            {
                node.text += v[i_rest] + " ";
            }
            if (!node.text.empty())
            {
                node.text.pop_back();
            }
            refreshNodeValues(node);
            nodes.push_back(std::move(node));
        }
    }

    for (auto& node1 : nodes)
    {
        for (auto& node2 : nodes)
        {
            for (auto& from : node1.out)
            {
                for (auto& to : node2.in)
                {
                    if (from == to)
                    {
                        node1.nexts.push_back(&node2);
                        node2.prevs.push_back(&node1);
                    }
                }
            }
        }
    }
    calPosition(nodes);
}

void ncnnLoader::nodesToFile(const std::deque<Node>& nodes, const std::string& filename)
{
    std::vector<Node*> nodes_turn;

    //lambda���������Ƿ��Ѿ���������
    auto contains = [&](std::vector<Node*>& v, Node* l) -> bool
    {
        return std::find(v.begin(), v.end(), l) != v.end();
    };

    //lambda�������ݹ齫��ѹ������
    //���һ������Ϊ�٣��������Ƿ�������ӣ�Ϊ�������ϸ���㴫��˳��
    std::function<void(Node*, int, std::vector<Node*>&, bool)> push_cal_stack = [&](Node* layer, int direct, std::vector<Node*>& stack, bool turn)
    {
        //�����Ӳ��ܻػ�
        if (layer == nullptr || contains(stack, layer))
        {
            return;
        }
        std::vector<Node*> connect0, connect1;
        connect1 = layer->nexts;
        connect0 = layer->prevs;

        if (direct < 0)
        {
            std::swap(connect0, connect1);
        }
        //ǰ��Ĳ㶼��ѹ�룬��ѹ�뱾��
        bool contain_all0 = true;
        for (auto& l : connect0)
        {
            if (!contains(stack, l))
            {
                contain_all0 = false;
                break;
            }
        }
        if (!turn || (!contains(stack, layer) && contain_all0))
        {
            stack.push_back(layer);
        }
        else
        {
            return;
        }
        for (auto& l : connect1)
        {
            push_cal_stack(l, direct, stack, turn);
        }
    };

    push_cal_stack((Node*)&nodes[0], 1, nodes_turn, true);

    std::vector<std::string> lines(2);
    lines[0] = "7767517";

    int blob_count = 0;
    for (auto& n : nodes_turn)
    {
        blob_count += n->nexts.size();
        auto l = fmt1::format("{:-16} {:-16} {} {} ", n->type, n->title, n->prevs.size(), n->nexts.size());
        for (auto& n1 : n->prevs)
        {
            if (n1->nexts.size() == 1)
            {
                l += n1->title + " ";
            }
            else
            {
                l += n1->title + "_" + n->title + " ";
            }
        }
        if (n->nexts.size() <= 1)
        {
            l += n->title + " ";
        }
        else
        {
            for (auto& n1 : n->nexts)
            {
                l += n->title + "_" + n1->title + " ";
            }
        }
        for (auto& kv : n->values)
        {
            l += kv.first + "=" + kv.second + " ";
        }
        l.pop_back();
        //l += convert::replaceAllSubString(n->text, "\n", " ");
        lines.push_back(std::move(l));
    }
    lines[1] = fmt1::format("{} {}", nodes_turn.size(), blob_count + 1);
    std::string str;
    for (auto& l : lines)
    {
        str += l + "\n";
    }
    fmt1::print("{}", str);
    convert::writeStringToFile(str, filename);
}

void ncnnLoader::refreshNodeValues(Node& n)
{
    auto strs = convert::splitString(n.text, " ");
    for (auto&str:strs)
    {
        auto kv = convert::splitString(str, "=");
        if (kv.size() >= 2)
        {
            n.values[kv[0]] = kv[1];
        }
    }
    n.text = "";
}

//std::vector<std::string> ncnnLoader::efftiveKeys(const std::string& type)
//{
//    return { "" };
//    if (type == "Convolution")
//    {
//        return { "channel", "window", "stride", "padding" };
//    }
//    else if (type == "Pooling")
//    {
//        return { "pooltype", "window", "stride", "padding" };
//    }
//    else if (type == "InnerProduct")
//    {
//        return { "node" };
//    }
//    else if (type == "input")
//    {
//        return { "data" };
//    }
//    return { "" };
//}
