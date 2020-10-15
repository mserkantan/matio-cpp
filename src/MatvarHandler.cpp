/*
 * Copyright (C) 2020 Fondazione Istituto Italiano di Tecnologia
 *
 * Licensed under either the GNU Lesser General Public License v3.0 :
 * https://www.gnu.org/licenses/lgpl-3.0.html
 * or the GNU Lesser General Public License v2.1 :
 * https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 * at your option.
 */

#include <matioCpp/MatvarHandler.h>
#include <matioCpp/ConversionUtilities.h>

matioCpp::MatvarHandler::Ownership::Ownership(std::weak_ptr<matvar_t *> pointerToDeallocate)
    : m_main(pointerToDeallocate)
{
    std::shared_ptr<matvar_t*> locked = pointerToDeallocate.lock();
    if (locked && *locked)
    {
        VariableType outputVariableType;
        ValueType outputValueType;
        if (matioCpp::get_types_from_matvart(*locked, outputVariableType, outputValueType))
        {
            if (outputVariableType == matioCpp::VariableType::CellArray)
            {
                size_t totalElements = 1;
                for (int dim = 0; dim < (*locked)->rank; ++dim)
                {
                    totalElements *= (*locked)->dims[dim];
                }

                m_ownedPointers.reserve(totalElements);
            }
        }
        m_ownedPointers.emplace(*locked);
    }

}

matioCpp::MatvarHandler::Ownership::~Ownership()
{
    dropAll();
}

bool matioCpp::MatvarHandler::Ownership::isOwning(matvar_t *test)
{
    return (test && ((test == *(m_main.lock())) || (m_ownedPointers.find(test) != m_ownedPointers.end())));
}

void matioCpp::MatvarHandler::Ownership::own(matvar_t *owned, DeleteMode mode)
{
    if (owned)
    {
        m_ownedPointers.insert(owned);

        if (mode == DeleteMode::Delete)
        {
            m_otherPointersToDeallocate.insert(owned);
        }
    }
}

void matioCpp::MatvarHandler::Ownership::drop(matvar_t *previouslyOwned)
{
    m_ownedPointers.erase(previouslyOwned);

    if (m_otherPointersToDeallocate.find(previouslyOwned) != m_otherPointersToDeallocate.end())
    {
        Mat_VarFree(previouslyOwned);
        m_otherPointersToDeallocate.erase(previouslyOwned);
    }
}

void matioCpp::MatvarHandler::Ownership::dropAll()
{
    std::shared_ptr<matvar_t*> locked = m_main.lock();
    if (locked)
    {
        if (*locked)
        {
            Mat_VarFree(*locked);
            *locked = nullptr;
        }
    }

    for(matvar_t* pointer : m_otherPointersToDeallocate)
    {
        Mat_VarFree(pointer);
    }

    m_otherPointersToDeallocate.clear();

    m_ownedPointers.clear();
}

matioCpp::MatvarHandler::MatvarHandler()
    : m_ptr(std::make_shared<matvar_t*>(nullptr))
{
}

matioCpp::MatvarHandler::MatvarHandler(matvar_t *inputPtr)
    : m_ptr(std::make_shared<matvar_t*>(inputPtr))
{

}

matioCpp::MatvarHandler::MatvarHandler(const matioCpp::MatvarHandler &other)
    : m_ptr(other.m_ptr)
{

}

matioCpp::MatvarHandler::MatvarHandler(matioCpp::MatvarHandler &&other)
    : m_ptr(other.m_ptr)
{

}

matvar_t *matioCpp::MatvarHandler::GetMatvarDuplicate(const matvar_t *inputPtr)
{
    std::string errorPrefix = "[ERROR][matioCpp::MatvarHandler::GetMatvarDuplicate] ";
    if (!inputPtr)
    {
        return nullptr;
    }

    VariableType outputVariableType;
    ValueType outputValueType;
    matvar_t* outputPtr;

    if (!matioCpp::get_types_from_matvart(inputPtr, outputVariableType, outputValueType))
    {
        std::cerr << errorPrefix << "The inputPtr is not supported." << std::endl;
        return nullptr;
    }

    if (outputVariableType == matioCpp::VariableType::CellArray) // It is a different case because Mat_VarDuplicate segfaults with a CellArray
    {
        matvar_t * shallowCopy = Mat_VarDuplicate(inputPtr, 0); // Shallow copy to remove const

        size_t totalElements = 1;

        for (int i = 0; i < inputPtr->rank; ++i)
        {
            totalElements *= inputPtr->dims[i];
        }

        std::vector<matvar_t*> vectorOfPointers(totalElements, nullptr);
        for (size_t i = 0; i < totalElements; ++i)
        {
            matvar_t* internalPointer = Mat_VarGetCell(shallowCopy, i);
            if (internalPointer)
            {
                vectorOfPointers[i] = GetMatvarDuplicate(internalPointer); //Deep copy
            }
        }

        outputPtr = Mat_VarCreate(inputPtr->name, inputPtr->class_type, inputPtr->data_type, inputPtr->rank, inputPtr->dims, vectorOfPointers.data(), 0);

        shallowCopy->data = nullptr; //This is a workaround for what it seems a matio problem.
            //When doing a shallow copy, the data is not copied,
            //but it will try to free it anyway with Mat_VarFree.
            //This avoids matio to attempt freeing the data
            // See https://github.com/tbeu/matio/issues/158
        Mat_VarFree(shallowCopy);
    }
    else
    {
        outputPtr = Mat_VarDuplicate(inputPtr, 1); //0 Shallow copy, 1 Deep copy
    }

    return outputPtr;

}
