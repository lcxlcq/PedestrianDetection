#include "myLBP.h"
#include <climits>

#ifndef NDEBUG
#   include <iomanip>
#endif

std::array<std::vector<bool>, myLBP::MAX_BIT_LENGTH / 8> myLBP::m_avbUniformMap = {};
std::array<std::vector<cv::Point2i>, myLBP::NUMBER_OF_PATTERNS> myLBP::m_SamplingPoints = {};

myLBP::myLBP(void) : myBlockBasedExtractor() {}

myLBP::myLBP(const cv::Mat& mImage, int Pattern, cv::Size2i BlockSize) :
    myBlockBasedExtractor(mImage, BlockSize) {
    Init();
    m_iPattern = Pattern;
    SetAttributes(Pattern);
}

myLBP::~myLBP(void) {}

void myLBP::Init(void) {
    m_bIsUniform = false;
    m_iRadius = 0;
    m_iLength = 0;

    if (m_SamplingPoints.at(0).empty() == true) {
        SetSamplingPoints();
        for (unsigned int iLength = 8; iLength <= myLBP::MAX_BIT_LENGTH; iLength += 8) {
            auto iUniformMapIndex = iLength / 8 - 1;
            myLBP::m_avbUniformMap.at(iUniformMapIndex).resize(1U << iLength);
            for (unsigned int j = 0; j < m_avbUniformMap.at(iUniformMapIndex).size(); ++j) {
                myLBP::m_avbUniformMap.at(iUniformMapIndex).at(j) = IsUniform(j, iLength);
            }
        }
    }
}

void myLBP::SetSamplingPoints(void) {
    using cv::Point2i;
    std::array<Point2i, 8> Location8_1 = {
        Point2i(-1, -1), Point2i(0, -1), Point2i(+1, -1), Point2i(+1, +0),
        Point2i(+1, +1), Point2i(0, +1), Point2i(-1, +1), Point2i(-1, +0),
    };
    for (auto pt : Location8_1) {
        m_SamplingPoints.at(Feature::LBP_8_1).push_back(pt);
    }

    std::array<Point2i, 8> Location8_2 = {
        Point2i(-2, -2), Point2i(+0, -2), Point2i(+2, -2), Point2i(+2, +0), 
        Point2i(+2, +2), Point2i(+0, +2), Point2i(-2, +2), Point2i(-2, +0)
    };
    for (auto pt : Location8_2) {
        m_SamplingPoints.at(Feature::LBP_8_2).push_back(pt);
    }

    std::array<Point2i, 16> Location16_2 = {
        Point2i(-2, -2), Point2i(-1, -2), Point2i(+0, -2), Point2i(+1, -2),
        Point2i(+2, -2), Point2i(+2, -1), Point2i(+2, +0), Point2i(+2, +1),
        Point2i(+2, +2), Point2i(+1, +2), Point2i(+0, +2), Point2i(-1, +2),
        Point2i(-2, +2), Point2i(-2, +1), Point2i(-2, +0), Point2i(-2, -1)
    };
    for (auto pt : Location16_2) {
        m_SamplingPoints.at(Feature::LBP_16_2).push_back(pt);
    }
}

void myLBP::SetAttributes(int iPattern) {
    m_bIsUniform = ((iPattern >= myLBP::NUMBER_OF_PATTERNS) ? true : false);
    m_iRadius = ((iPattern % myLBP::NUMBER_OF_PATTERNS == 0) ? 1 : 2);
    m_iLength = ((iPattern % myLBP::NUMBER_OF_PATTERNS == 2) ? 16 : 8);
}

void myLBP::Describe(cv::Point2i Position, std::vector<float>& vfFeature) const {
    auto iUniformMapIndex = m_iLength / 8 - 1;
    std::vector<float> viTempBins(m_avbUniformMap.at(iUniformMapIndex).size(),
                                  0.0f);

    for (int y = Position.y; y < Position.y + m_BlockSize.height; ++y) {
        for (int x = Position.x; x < Position.x + m_BlockSize.width; ++x) {
            auto BinNumber = GetBinNumber(cv::Point2i(x, y));
            if (BinNumber != UINT_MAX) {
                ++viTempBins.at(BinNumber);
            }
        }
    }

    vfFeature.clear();
    float iNonuniformBin = 0.0f;
    for (std::size_t i = 0; i < myLBP::m_avbUniformMap.at(iUniformMapIndex).size(); ++i) {
        if (m_bIsUniform == true) {
            if (myLBP::m_avbUniformMap.at(iUniformMapIndex).at(i) == true) {
                vfFeature.push_back(viTempBins.at(i));
            } else {
                iNonuniformBin += viTempBins.at(i);
            }
        } else {
            vfFeature.push_back(viTempBins.at(i));
        }
    }

    if (m_bIsUniform == true) {
        vfFeature.push_back(iNonuniformBin);
    }
}

unsigned int myLBP::GetBinNumber(cv::Point2i Position) const {
    cv::Point2i ptRadius(m_iRadius, m_iRadius);
    cv::Point2i ptLeftTop(Position - ptRadius);
    cv::Point2i ptBottomRight(Position + ptRadius + cv::Point2i(1, 1));
    if (ptLeftTop.x < 0 || ptBottomRight.x >= m_mImage.cols ||
        ptLeftTop.y < 0 || ptBottomRight.y >= m_mImage.rows) {
        return UINT_MAX;
    }

    return GetBinNumber(m_mImage(cv::Rect2i(ptLeftTop, ptBottomRight)));
}

unsigned int myLBP::GetBinNumber(const cv::Mat& mImg) const {
    unsigned int iBinNumber = 0;
    
    const cv::Point2i ptCenter(m_iRadius, m_iRadius);
    const auto cCenterIntensity = mImg.at<unsigned char>(ptCenter);
    const auto& SampleingPoints =
        m_SamplingPoints.at(m_iPattern % myLBP::NUMBER_OF_PATTERNS);
    for (const auto pt : SampleingPoints) {
        auto cCurrentIntensity = mImg.at<unsigned char>(ptCenter + pt);
        iBinNumber = (iBinNumber << 1) |
            ((cCurrentIntensity <= cCenterIntensity) ? 0x00 : 0x01);
    }

    return iBinNumber;
}

bool myLBP::IsUniform(int iPatten, unsigned int iBinNumber) {
    auto iIndex = iPatten == Feature::LBP_16_2_UNIFORM ? 1 : 0;
    return m_avbUniformMap.at(iIndex).at(iBinNumber);
}

bool myLBP::IsUniform(unsigned int iBinNumber, unsigned int iLength) {
    bool bResult = true;

    int iChangeTime = 0;
    unsigned int iMaskBit = 0x00000001;
    unsigned int iCheckBit = iBinNumber & iMaskBit;

    for (unsigned int i = 0; i < iLength - 1; ++i) {
        iMaskBit <<= 1;
        iCheckBit <<= 1;
        unsigned int iComparedValue = (iMaskBit & iBinNumber);
        if (iComparedValue != iCheckBit) {
            iCheckBit = iComparedValue;
            ++iChangeTime;
            if (iChangeTime > myLBP::MAX_TRANSITION_TIME) {
                bResult = false;
                break;
            }
        }
    }

    return bResult;
}

#ifndef NDEBUG
void myLBP::PrintUniformMap(int iLength) const {
    int i = 0;
    int sum = 0;
    for (auto bIsUnoform : m_avbUniformMap.at(iLength / 8 - 1)) {
        std::cout << std::setw(3) << i++ << ":" << (bIsUnoform ? "T" : "F") << std::endl;
        if (bIsUnoform == true) {
            ++sum;
        }
    }
    std::cout << sum << std::endl;
}
#endif