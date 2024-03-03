#include "Solver.h"
#include "Global.h"
#include <iostream>
#include <algorithm>

namespace Fluid2d {

    Solver::Solver(ParticalSystem& ps) : mPs(ps), mW(ps.mSupportRadius)
    {

    }

    Solver::~Solver() {

    }

    void Solver::Iterate() {
        Glb::Timer timer;
        timer.Start();
        UpdateDensityAndPressure();
        InitAccleration();
        UpdateViscosityAccleration();
        UpdatePressureAccleration();
        EulerIntegrate();
        BoundaryCondition();
        //std::cout << "solve time = " << timer.GetTime() << std::endl;
    }

    void Solver::UpdateDensityAndPressure() {
        mPs.mDensity = std::vector<float>(mPs.mPositions.size(), Para::density0);
        mPs.mPressure = std::vector<float>(mPs.mPositions.size(), 0.0f);
        for (int i = 0; i < mPs.mPositions.size(); i++) {    // ����������
            if (mPs.mNeighbors.size() != 0) {    // ���ھ�
                std::vector<NeighborInfo>& neighbors = mPs.mNeighbors[i];
                float density = 0;
                for (auto& nInfo : neighbors) {
                    density += mW.Value(nInfo.distance);
                }
                density *= (mPs.mVolume * Para::density0);
                mPs.mDensity[i] = density;
                mPs.mDensity[i] = std::max(density, Para::density0);        // ��ֹ����
            }
            // ����ѹǿ
            mPs.mPressure[i] = mPs.mStiffness* (std::powf(mPs.mDensity[i] / Para::density0, mPs.mExponent) - 1.0f);
        }
    }

    void Solver::InitAccleration() {
        std::fill(mPs.mAccleration.begin() + mPs.mStartIndex, mPs.mAccleration.end(), glm::vec2(0.0f, -Para::gravity));
    }

    void Solver::UpdateViscosityAccleration() {
        float dim = 2.0f;
        float constFactor = 2.0f * (dim + 2.0f) * mPs.mViscosity;
        for (int i = mPs.mStartIndex; i < mPs.mPositions.size(); i++) {    // ����������
            if (mPs.mNeighbors.size() != 0) {    // ���ھ�
                std::vector<NeighborInfo>& neighbors = mPs.mNeighbors[i];
                glm::vec2 viscosityForce(0.0f, 0.0f);
                for (auto& nInfo : neighbors) {
                    int j = nInfo.index;
                    float dotDvToRad = glm::dot(mPs.mVelocity[i] - mPs.mVelocity[j], nInfo.radius);

                    float denom = nInfo.distance2 + 0.01f * mPs.mSupportRadius2;
                    viscosityForce += (mPs.mMass / mPs.mDensity[j]) * dotDvToRad * mW.Grad(nInfo.radius) / denom;
                }
                viscosityForce *= constFactor;
                mPs.mAccleration[i] += viscosityForce;
            }
        }
    }

    void Solver::UpdatePressureAccleration() {
        std::vector<float> pressDivDens2(mPs.mPositions.size(), 0);        // p/(dens^2)
        for (int i = 0; i < mPs.mPositions.size(); i++) {
            pressDivDens2[i] = mPs.mPressure[i] / std::powf(mPs.mDensity[i], 2);
        }

        for (int i = mPs.mStartIndex; i < mPs.mPositions.size(); i++) {    // ����������
            if (mPs.mNeighbors.size() != 0) {    // ���ھ�
                std::vector<NeighborInfo>& neighbors = mPs.mNeighbors[i];
                glm::vec2 pressureForce(0.0f, 0.0f);
                for (auto& nInfo : neighbors) {
                    int j = nInfo.index;
                    pressureForce += mPs.mDensity[j] * (pressDivDens2[i] + pressDivDens2[j]) * mW.Grad(nInfo.radius);
                }
                mPs.mAccleration[i] -= pressureForce * mPs.mVolume;
            }
        }
    }

    void Solver::EulerIntegrate() {
        for (int i = mPs.mStartIndex; i < mPs.mPositions.size(); i++) {    // ����������
            mPs.mVelocity[i] += Para::dt * mPs.mAccleration[i];
            mPs.mVelocity[i] = glm::clamp(mPs.mVelocity[i], glm::vec2(-100.0f), glm::vec2(100.0f));
            mPs.mPositions[i] += Para::dt * mPs.mVelocity[i];
        }
    }

    void Solver::BoundaryCondition() {
        for (int i = 0; i < mPs.mPositions.size(); i++) {    // ����������
            glm::vec2& position = mPs.mPositions[i];
            bool invFlag = false;
            if (position.y < mPs.mLowerBound.y + mPs.mSupportRadius) {
                mPs.mVelocity[i].y = std::abs(mPs.mVelocity[i].y);
                invFlag = true;
            }

            if (position.y > mPs.mUpperBound.y - mPs.mSupportRadius) {
                mPs.mVelocity[i].y = -std::abs(mPs.mVelocity[i].y);
                invFlag = true;
            }

            if (position.x < mPs.mLowerBound.x + mPs.mSupportRadius) {
                mPs.mVelocity[i].x = std::abs(mPs.mVelocity[i].x);
                invFlag = true;
            }

            if (position.x > mPs.mUpperBound.x - mPs.mSupportRadius) {
                mPs.mVelocity[i].x = -std::abs(mPs.mVelocity[i].x);
                invFlag = true;
            }

            if (invFlag) {
                mPs.mPositions[i] += Para::dt * mPs.mVelocity[i];
                mPs.mVelocity[i] = glm::clamp(mPs.mVelocity[i], glm::vec2(-100.0f), glm::vec2(100.0f));    // �ٶ�����
            }
        }
    }

}

