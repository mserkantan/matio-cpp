/*
 * Copyright (C) 2020 Fondazione Istituto Italiano di Tecnologia
 *
 * Licensed under either the GNU Lesser General Public License v3.0 :
 * https://www.gnu.org/licenses/lgpl-3.0.html
 * or the GNU Lesser General Public License v2.1 :
 * https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 * at your option.
 */


#include <matioCpp/Variable.h>

class matioCpp::Variable::Impl
{
public:
    matvar_t* matVar_ptr{nullptr};
    std::vector<size_t> dimensions;

    void freePtr()
    {
        if (matVar_ptr)
        {
            Mat_VarFree(matVar_ptr);
            matVar_ptr = nullptr;
        }
    }

    void resetPtr(matvar_t* newPtr)
    {
        freePtr();
        matVar_ptr = newPtr;
    }

    Impl()
    { }

    ~Impl()
    {
        freePtr();
    }
};

bool matioCpp::Variable::createVar(const std::string& name, const VariableType& variableType, const ValueType& valueType, const std::vector<size_t>& dimensions, void* data)
{
    std::string errorPrefix = "[ERROR][matioCpp::Variable::createVar] ";
    if (name.empty())
    {
        std::cerr << errorPrefix << "The name should not be empty." << std::endl;
        return false;
    }

    if (dimensions.size() < 2)
    {
        std::cerr << errorPrefix << "The dimensions should be at least 2." << std::endl;
        return false;
    }

    if (!data)
    {
        std::cerr << errorPrefix << "The data pointer is empty." << std::endl;
        return false;
    }

    matio_types matioType;
    matio_classes matioClass;

    if(!get_matio_types(variableType, valueType, matioClass, matioType))
    {
        std::cerr << errorPrefix << "Either the variableType or the valueType are not supported." << std::endl;
        return false;
    }

    m_pimpl->dimensions = dimensions;

    m_pimpl->resetPtr(Mat_VarCreate(name.c_str(), matioClass, matioType, dimensions.size(), m_pimpl->dimensions.data(), data, 0));

    if (!m_pimpl->matVar_ptr)
    {
        std::cerr << errorPrefix << "Failed to create the variable." << std::endl;
        return false;
    }

    return true;
}

bool matioCpp::Variable::createComplexVar(const std::string& name, const VariableType& variableType, const ValueType& valueType, const std::vector<size_t>& dimensions, void *realData, void *imaginaryData)
{
    std::string errorPrefix = "[ERROR][matioCpp::Variable::createComplexVar] ";
    if (name.empty())
    {
        std::cerr << errorPrefix << "The name should not be empty." << std::endl;
        return false;
    }

    if (dimensions.size() < 2)
    {
        std::cerr << errorPrefix << "The dimensions should be at least 2." << std::endl;
        return false;
    }

    if (!realData)
    {
        std::cerr << errorPrefix << "The real data pointer is empty." << std::endl;
        return false;
    }

    if (!imaginaryData)
    {
        std::cerr << errorPrefix << "The imaginary data pointer is empty." << std::endl;
        return false;
    }

    matio_types matioType;
    matio_classes matioClass;

    if (!get_matio_types(variableType, valueType, matioClass, matioType))
    {
        std::cerr << errorPrefix << "Either the variableType or the valueType are not supported." << std::endl;
        return false;
    }

    mat_complex_split_t matioComplexSplit;
    matioComplexSplit.Re = realData;
    matioComplexSplit.Im = imaginaryData;

    m_pimpl->resetPtr(Mat_VarCreate(name.c_str(), matioClass, matioType, m_pimpl->dimensions.size(), m_pimpl->dimensions.data(), &matioComplexSplit, MAT_F_COMPLEX)); //Data is hard copied, since the flag MAT_F_DONT_COPY_DATA is not used

    return m_pimpl->matVar_ptr != nullptr;
}

matioCpp::Variable::Variable()
    : m_pimpl(std::make_unique<Impl>())
{

}

matioCpp::Variable::Variable(const matioCpp::Variable &other)
{
    this->operator=(other);
}

matioCpp::Variable::Variable(matioCpp::Variable &&other)
{
    this->operator=(other);
}

matioCpp::Variable::~Variable()
{

}

matioCpp::Variable &matioCpp::Variable::operator=(const matioCpp::Variable &other)
{
    fromMatio(other.m_pimpl->matVar_ptr);
    return *this;
}

matioCpp::Variable &matioCpp::Variable::operator=(matioCpp::Variable &&other)
{
    m_pimpl = std::move(other.m_pimpl);
    return *this;
}

bool matioCpp::Variable::fromMatio(const matvar_t *inputVar)
{
    std::string errorPrefix = "[ERROR][matioCpp::Variable::fromMatio] ";
    if (!inputVar)
    {
        std::cerr << errorPrefix << "Empty input variable." << std::endl;
        return false;
    }

    matioCpp::Span<size_t> dimensionsSpan = matioCpp::make_span(inputVar->dims, inputVar->rank);
    m_pimpl->dimensions.assign(dimensionsSpan.begin(), dimensionsSpan.end());

    m_pimpl->resetPtr(Mat_VarDuplicate(inputVar, 0));

    return true;
}

const matvar_t *matioCpp::Variable::toMatio() const
{
    return m_pimpl->matVar_ptr;
}

std::string matioCpp::Variable::name() const
{
    if (m_pimpl->matVar_ptr)
    {
        return m_pimpl->matVar_ptr->name;
    }
    else
    {
        return "";
    }
}

matioCpp::VariableType matioCpp::Variable::variableType() const
{
    matioCpp::VariableType outputVariableType = matioCpp::VariableType::Unsupported;
    matioCpp::ValueType outputValueType = matioCpp::ValueType::UNSUPPORTED;
    get_types_from_matvart(m_pimpl->matVar_ptr, outputVariableType, outputValueType);
    return outputVariableType;
}

matioCpp::ValueType matioCpp::Variable::valueType() const
{
    matioCpp::VariableType outputVariableType = matioCpp::VariableType::Unsupported;
    matioCpp::ValueType outputValueType = matioCpp::ValueType::UNSUPPORTED;
    get_types_from_matvart(m_pimpl->matVar_ptr, outputVariableType, outputValueType);
    return outputValueType;
}

bool matioCpp::Variable::isComplex() const
{
    if (m_pimpl->matVar_ptr)
    {
        return m_pimpl->matVar_ptr->isComplex;
    }
    else
    {
        return false;
    }
}
