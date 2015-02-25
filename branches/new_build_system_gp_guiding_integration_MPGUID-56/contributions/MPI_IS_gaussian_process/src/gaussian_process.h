/*
 * Copyright 2014-2015, Max Planck Society.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software without 
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*!@file
 * @author  Klenske <edgar.klenske@tuebingen.mpg.de>
 * @author  Stephan Wenninger <stephan.wenninger@tuebingen.mpg.de>
 *
 * @brief
 * The GP class implements the Gaussian Process functionality.
 *
 */

#ifndef GAUSSIAN_PROCESS_H
#define GAUSSIAN_PROCESS_H

#include <Eigen/Dense>
#include <vector>
#include <list>
#include <memory>
#include <utility>
#include <cstdint>
#include "covariance_functions.h"
#include "parameter_priors.h"

// Constants

// Jitter is the minimal "noise" on the otherwise noiseless kernel matrices to
// make the Cholesky decomposition stable.
#define JITTER 1e-6

class GP {
private:
  covariance_functions::CovFunc* covFunc_;
  Eigen::VectorXd data_loc_;
  Eigen::VectorXd data_out_;
  Eigen::MatrixXd gram_matrix_;
  std::vector<Eigen::MatrixXd> gram_matrix_derivatives_;
  Eigen::VectorXd alpha_;
  Eigen::LDLT<Eigen::MatrixXd> chol_gram_matrix_;
  double log_noise_sd_;
  Eigen::VectorXi optimization_mask_;
  std::vector<parameter_priors::ParameterPrior*> prior_vector_;

public:
  typedef std::pair<Eigen::VectorXd, Eigen::MatrixXd> VectorMatrixPair;

  GP(); // allowing the standard constructor makes the use so much easier!
  GP(const covariance_functions::CovFunc& covFunc);
  GP(const double noise_variance,
     const covariance_functions::CovFunc& covFunc);
  ~GP(); // Need to tidy up
  
  GP(const GP& that);
  GP& operator=(const GP& that);

  /*! Sets the covariance function
   *
   * This operation is possible only if there is not inference going on in the
   * current instance. This is useful after initialisation.
   */
  bool setCovarianceFunction(const covariance_functions::CovFunc& covFunc);


  /*!
   * Returns a GP sample for the given locations.
   *
   * Returns a sample of the prior if the Gram matrix is empty.
   */
  Eigen::VectorXd drawSample(const Eigen::VectorXd& locations) const;

  /*!
   * Returns a sample of the GP based on a given vector of random numbers.
   */
  Eigen::VectorXd drawSample(const Eigen::VectorXd& locations,
                             const Eigen::VectorXd& random_vector) const;

  /*!
   * Builds an inverts the Gram matrix for a given set of datapoints.
   *
   * This function works on the already stored data and doesn't return
   * anything. The work is done here, I/O somewhere else.
   */
  void infer();

  /*!
   * Stores the given datapoints in the form of data location \a data_loc and
   * the output values \a data_out. Calls infer() everytime so that the Gram
   * matrix is rebuild and the Cholesky decomposition is computed.
   */
  void infer(const Eigen::VectorXd& data_loc,
             const Eigen::VectorXd& data_out);

  /*!
   * Sets the GP back to the prior:
   * Removes datapoints, empties the Gram matrix.
   */
  void clear();

  /*!
   * Predicts the mean and covariance for a vector of locations.
   *
   * This function just builds the prior and mixed covariance matrices and
   * calls the other predict afterwards.
   */
  VectorMatrixPair predict(const Eigen::VectorXd& locations) const;

  /*!
   * Does the real work for predict. Solves the Cholesky decomposition for the
   * given matrices. The Gram matrix and measurements need to be cached
   * already.
   */
  VectorMatrixPair predict(const Eigen::MatrixXd& prior_cov,
                           const Eigen::MatrixXd& mixed_cov) const;

  /*!
   * Combines the likelihood and the prior to obtain the posterior.
   */
  double neg_log_posterior() const;

  /*!
   * Derivative of the log posterior.
   */
  Eigen::VectorXd neg_log_posterior_gradient() const;

  /*!
   * Calculates the negative log likelihood.
   *
   * This function is used for model selection and optimization of hyper
   * parameters. The calculations are completely done on the cached datapoints.
   */
  double neg_log_likelihood() const;

  /*!
   * Calculates the derivative of the negative log likelihood.
   *
   * This function is used for model selection and optimization of hyper
   * parameters. The calculations are completely done on the cached datapoints.
   */
  Eigen::VectorXd neg_log_likelihood_gradient() const;

  /*!
   * A wrapper that combines the likelihood and gradient in a form that is
   * compatible to the BFGS optimizer.
   */
  //void evaluate(const Eigen::VectorXd& x,
  //              double* function_value, Eigen::VectorXd* derivative);

  /*!
   * Sets the hyperparameters to the given vector.
   */
  void setHyperParameters(const Eigen::VectorXd& hyperParameters);

  /*!
   * Returns the hyperparameters to the given vector.
   */
  Eigen::VectorXd getHyperParameters() const;

  /*!
   * Sets the covariance hyperparameters to the given vector.
   */
  void setCovarianceHyperParameters(const Eigen::VectorXd& hyperParameters);

  /*!
   * Optimizes the hyperparameters for a certain number of line searches
   */
  Eigen::VectorXd optimizeHyperParameters(int number_of_linesearches) const;

  /*!
   * Sets the optimization mask to determine which parameters should be
   * optimized. Think of it as a delta-peak prior.
   */
  void setOptimizationMask(const Eigen::VectorXi& mask);

  /*!
   * Unsets the optimization mask.
   */
  void clearOptimizationMask();

  /*!
   * Uses the stored mask and hyperparameters to create a vector of those
   * parameters that are not masked.
   */
  Eigen::VectorXd mask(const Eigen::VectorXd& original_parameters);

  /*!
   * Uses the mask and the supplied parameter vector to create a full parameter
   * vector, where the supplied parameters are inserted at the places according
   * to the mask.
   */
  Eigen::VectorXd unmask(const Eigen::VectorXd& masked_parameters);

  /*!
   * Sets a hyperprior for a certain hyperparameter (addressed by the
   * parameter's index).
   */
  void setHyperPrior(const parameter_priors::ParameterPrior& prior, int index);

  /*!
   * Removes a hyperprior for a given parameter index.
   */
  void clearHyperPrior(int index);



};

#endif  // ifndef GAUSSIAN_PROCESS_H
