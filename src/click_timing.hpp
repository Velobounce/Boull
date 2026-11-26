#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <random>

// Shared interval generator; state belongs to each caller.
namespace ClickTiming {
// A dispatch-time ceiling, shared by every click path of one caller. Keep
// this state across target changes and short pauses; elapsed samples expire.
template<class Clock>
class WindowBudget {
public:
    bool TryConsume(typename Clock::time_point now, int maxCps)
    {
        const auto window = std::chrono::seconds(1);
        while (m_Size > 0 && now - m_Samples[m_Head] >= window) {
            m_Head = (m_Head + 1) % m_Samples.size();
            --m_Size;
        }
        const auto limit = static_cast<std::size_t>((std::clamp)(maxCps, 1, 50));
        if (m_Size >= limit) return false;
        m_Samples[(m_Head + m_Size) % m_Samples.size()] = now;
        ++m_Size;
        return true;
    }

private:
    std::array<typename Clock::time_point, 50> m_Samples{};
    std::size_t m_Head = 0;
    std::size_t m_Size = 0;
};

template<class TimePoint, class Duration>
TimePoint AdvanceDeadline(TimePoint deadline, TimePoint now, Duration interval)
{
    const auto next = deadline + interval;
    return next > now ? next : now + interval;
}

template<class Clock, class Rng>
typename Clock::duration Next(int minCps, int maxCps, Rng& m_Rng,
    double& m_SmoothedCps, double& m_TargetCps,
    int& m_TargetClicksRemaining, bool& m_CpsStateInitialized)
{
    minCps = (std::max)(1, minCps);
    maxCps = (std::max)(minCps, maxCps);
    const double minValue =
        static_cast<double>(minCps);

    const double maxValue =
        static_cast<double>(maxCps);

    if (minCps == maxCps)
    {
        m_SmoothedCps = minValue;
        m_TargetCps = minValue;
        m_TargetClicksRemaining = 0;
        m_CpsStateInitialized = true;

        return std::chrono::duration_cast<
            typename Clock::duration>(
            std::chrono::duration<double>(
                1.0 / minValue));
    }

    const double cpsRange =
        maxValue - minValue;

    if (!m_CpsStateInitialized ||
        m_SmoothedCps < minValue ||
        m_SmoothedCps > maxValue)
    {
        m_SmoothedCps =
            std::uniform_real_distribution<double>(
                minValue,
                maxValue)(m_Rng);

        m_TargetCps = m_SmoothedCps;
        m_TargetClicksRemaining = 0;
        m_CpsStateInitialized = true;
    }

    /*
     * O ritmo trabalha em pequenos "blocos" correlacionados. Em vez de
     * escolher um CPS aleatorio completamente novo por clique, escolhemos
     * um alvo e caminhamos ate ele durante alguns cliques. Isso produz
     * aceleracoes/desaceleracoes perceptiveis sem saltos artificiais.
     */
    if (m_TargetClicksRemaining <= 0)
    {
        const double phase =
            std::uniform_real_distribution<double>(
                0.0,
                1.0)(m_Rng);

        if (phase < 0.24)
        {
            // Periodo naturalmente mais lento, ainda dentro do Min CPS.
            m_TargetCps =
                std::uniform_real_distribution<double>(
                    minValue,
                    minValue + cpsRange * 0.32)(m_Rng);
        }
        else if (phase > 0.76)
        {
            // Periodo naturalmente mais rapido, sem ultrapassar Max CPS.
            m_TargetCps =
                std::uniform_real_distribution<double>(
                    maxValue - cpsRange * 0.32,
                    maxValue)(m_Rng);
        }
        else
        {
            m_TargetCps =
                std::uniform_real_distribution<double>(
                    minValue + cpsRange * 0.12,
                    maxValue - cpsRange * 0.12)(m_Rng);
        }

        m_TargetClicksRemaining =
            std::uniform_int_distribution<int>(
                3,
                9)(m_Rng);
    }

    --m_TargetClicksRemaining;

    const double followStrength =
        std::uniform_real_distribution<double>(
            0.26,
            0.46)(m_Rng);

    m_SmoothedCps +=
        (m_TargetCps - m_SmoothedCps) * followStrength;

    // Pequeno drift continuo evita linhas/platôs matematicos no grafico.
    const double driftAmplitude =
        (std::max)(0.035, cpsRange * 0.028);

    m_SmoothedCps +=
        std::normal_distribution<double>(
            0.0,
            driftAmplitude)(m_Rng);

    m_SmoothedCps =
        (std::clamp)(
            m_SmoothedCps,
            minValue,
            maxValue);

    // Variacao curta por clique. Continua limitada ao Min/Max configurado.
    const double clickNoiseAmplitude =
        (std::max)(0.045, cpsRange * 0.035);

    double instantCps =
        m_SmoothedCps +
        std::normal_distribution<double>(
            0.0,
            clickNoiseAmplitude)(m_Rng);

    instantCps =
        (std::clamp)(
            instantCps,
            minValue,
            maxValue);

    return std::chrono::duration_cast<
        typename Clock::duration>(
        std::chrono::duration<double>(
            1.0 / instantCps));
}
}

